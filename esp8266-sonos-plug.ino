#include <ESP8266WiFi.h>

//+=============================================================================
// Customize the following settings
//
const char* ssid = "WiFi"; // WiFi SSID
const char* password = "WiFi_password"; // WiFi password
const char* passcode = "pass"; // Access code to send IR commands
const int port = 8081; //  Receiving HTTP port
//
// End configuration area
//+=============================================================================


//+=============================================================================
// Connect to WiFi
//
void setup() {
  // Initialize serial
  Serial.begin(115200);
  Serial.println("ESP8266 IR Controller");

  // Begin WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void loop() {
}
