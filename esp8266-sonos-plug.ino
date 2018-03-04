#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SonosUPnP.h>

//+=============================================================================
// Customize the following settings
//
const char* ssid = ""; // WiFi SSID
const char* password = ""; // WiFi password

String plug_ip = "192.168.1.109";

String sonos_ip = "192.168.1.104";
String sonos_mac = "B8-E9-37-93-F9-60";
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
// Connect to WiFi
//
void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println("ESP8266 Sonos Plug");

  // Begin WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

}

//+=============================================================================
// Loop over checking Sonos, powering plug
//
void loop() {
  if (is_on()) {
    Serial.println("Turning OFF");
    turn_off();
  } else {
    Serial.println("Turning ON");
    turn_on();
  }
  
  delay(5000);
}
