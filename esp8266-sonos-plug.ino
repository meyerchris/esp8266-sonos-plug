#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SonosUPnP.h>

//+=============================================================================
// Customize the following settings
//
const char* ssid = ""; // WiFi SSID
const char* password = ""; // WiFi password

String plug_ip = "192.168.1.109";

IPAddress g_sonosIP(192, 168, 1, 104);
const char* g_sonosMAC = "B8E93793F960";
//
// End configuration area
//+=============================================================================

//+=============================================================================
// Global variables
//
void ethConnectError() {
  Serial.println("Couldn't connect to the network, bail!");
}

WiFiClient g_client;
SonosUPnP g_sonos = SonosUPnP(g_client, ethConnectError);

#define SONOS_STATUS_POLL_DELAY_MS 5000
// Delay before turning plug off (5 [min] * 60 [sec/min] * 1000 [msec/sec] = 300000 [msec])
#define PLUG_POWER_DELAY_MS 300000
unsigned long g_sonosLastStateUpdate = 0;
unsigned long g_sonosLastTimePlaying = 0;

//+=============================================================================
// Helper functions
//
String encrypt(String request) {
  int key = 171;
  std::vector<char> buff;

  // the first four bytes are the big-endian length of the command
  int len = request.length();
  buff.push_back( (len >> 24) & 0xff );
  buff.push_back( (len >> 16) & 0xff );
  buff.push_back( (len >> 8) & 0xff );
  buff.push_back( len & 0xff );

  // the remaining bytes are the encrypted letters in the request
  for (int i = 0; i < len; ++i) {
    int letter = request[i];
    char cypher = key ^ letter;
    key = cypher;
    buff.push_back(cypher);
  }

  String command;
  for (int i = 0; i < buff.size(); ++i) {
    command.concat(buff[i]);
  }

  return command;
}

String decrypt(std::vector<char> buff) {
  int key = 171;

  std::vector<char> plain_buff;
  for (int i = 4; i < buff.size(); ++i) {
    char plain = key ^ buff[i];
    key = buff[i];
    plain_buff.push_back(plain);
  }

  String response;
  for (int i = 0; i < plain_buff.size(); ++i) {
    response.concat(plain_buff[i]);
  }

  return response;
}

String send_command(String command) {

  WiFiClient socket;
  if (!socket.connect(plug_ip, 9999)) {
    Serial.println("Couldn't connect!");
  }

  // Serial.println("Connected...");

  String payload = encrypt(command);

  socket.print(payload);
  // Serial.println("Sent data...");

  unsigned long timeout = millis();
  while (socket.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Timeout waiting for response!");
      socket.stop();
      delay(1000);
      return "";
    }
  }
  // Serial.println("Received response:");

  std::vector<char> response;
  while (socket.available()) {
    String line = socket.readStringUntil('\r');
    for (int i = 0; i < line.length(); ++i) {
      response.push_back(line[i]);
    }
  }

  String answer = "No response...";
  if (response.size() >= 4) {
    answer = decrypt(response);
  } else {
    Serial.println("Response too small!");
    return "";
  }

  // Serial.println(answer);
  // Serial.println("Closing connecing.");

  return answer;
}

bool is_on() {
  String command = "{\"system\":{\"get_sysinfo\":{}}}";
  String response = send_command(command);

  DynamicJsonBuffer jsonBuffer;  
  JsonObject& jason = jsonBuffer.parseObject(response);
  int state = jason["system"]["get_sysinfo"]["relay_state"];

  return state;
}

bool turn_on() {
  String command = "{\"system\": {\"set_relay_state\": {\"state\": 1}}}";
  String response = send_command(command);

  DynamicJsonBuffer jsonBuffer;  
  JsonObject& jason = jsonBuffer.parseObject(response);
  int state = jason["system"]["set_relay_state"]["err_code"];

  return state;
}

bool turn_off() {
  String command = "{\"system\": {\"set_relay_state\": {\"state\": 0}}}";
  String response = send_command(command);

  DynamicJsonBuffer jsonBuffer;  
  JsonObject& jason = jsonBuffer.parseObject(response);
  int state = jason["system"]["set_relay_state"]["err_code"];

  return state;
}

//+=============================================================================
// Check if WiFi is connected, start if not
//
void check_wifi() {
  if (WiFi.status() ==  WL_CONNECTED) {
    return;
  } else {
    // Begin WiFi
    Serial.print("Beginning WiFi connection");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected, local IP:");
    Serial.println(WiFi.localIP());

    // Start countdown for turning off plug
    g_sonosLastTimePlaying = millis();
  }
}

//+=============================================================================
// Connect to WiFi
//
void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println("ESP8266 Sonos Plug");

  check_wifi();
}

//+=============================================================================
// Loop over checking Sonos, powering plug
//
void loop() {

  if (g_sonosLastStateUpdate > millis() || millis() > g_sonosLastStateUpdate + SONOS_STATUS_POLL_DELAY_MS) {

    // First make sure we have an internet connection
    check_wifi();

    // bool plugIsOn = is_on();
    bool sonosIsPlaying = g_sonos.getState(g_sonosIP) == SONOS_STATE_PLAYING;
    bool sonosIsMuted = g_sonos.getMute(g_sonosIP);

    Serial.println("");

    Serial.print("Sonos is playing: ");
    Serial.println(sonosIsPlaying);

    Serial.print("Sonos is muted: ");
    Serial.println(sonosIsMuted);

    // Serial.print("Plug is on: ");
    // Serial.println(plugIsOn);

    Serial.println("");

    if (sonosIsPlaying && !sonosIsMuted) {
      // Check if Sonos is playing and:
      // - Keep track of most recent time it was playing
      // - If plug isn't on and music is playing, turn it on!

      g_sonosLastTimePlaying = millis();

      // if (!plugIsOn) {
      //   Serial.println("Sonos is playing but plug is off. Turning it on.");
      //   turn_on();
      // }
      Serial.println("Sonos is playing, turning plug on (may already be on).");
      turn_on();
    // } else if (plugIsOn) {
    } else {
      // If Sonos is not playing, but the plug is on:
      // - Turn the plug off if it's been more than 5 minutes
      // - If plug isn't on and music is playing, turn it on!

      unsigned long timeSincePlayingS = (millis() - g_sonosLastTimePlaying)/1000;
      Serial.print("Sonos isn't playing but the plug is on. It's been ");
      Serial.print(timeSincePlayingS);
      Serial.println(" seconds since something last played.");

      if (g_sonosLastTimePlaying > millis() || millis() > g_sonosLastTimePlaying + PLUG_POWER_DELAY_MS) {
        Serial.println("--> Turning off plug.");
        turn_off();
      }
    // } else {
    //   // Sonos isn't playing, and plug is off. Nothing to do.
    //   unsigned long timeSincePlayingS = (millis() - g_sonosLastTimePlaying)/1000;
    //   Serial.print("Sonos isn't playing and the plug is off. It's been ");
    //   Serial.print(timeSincePlayingS);
    //   Serial.println(" seconds since something last played.");
    }


    g_sonosLastStateUpdate = millis();
  }

  delay(1000);
}
