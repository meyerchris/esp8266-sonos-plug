# esp8266-sonos-plug

## Synopsis

This repositor will eventually control a tp-link wireless outlet based on the play-state of a Sonos.
If an amplifier is plugged into the outlet, it will only be powered when the specified Sonos is playing music.

## Instructions

You will need to download this software using some form of Git.
If you don't have a native version with your OS, you can use the [GitHub desktop client](https://desktop.github.com/).

Once you have the software locally, you will need to compile it using the [Arduino IDE](https://www.arduino.cc/en/Main/Software).
A simple list of the setup steps to use this with the ESP8266 board is:
1. Install [Arduino IDE](https://www.arduino.cc/en/Main/Software)
1. Install [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
1. Install [Node MCU drivers](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx) if they're not detected
1. Install the following libraries from the following libraries from the Arduino IDE [Library Manager](https://www.arduino.cc/en/Guide/Libraries):
  * `ESP8266WiFi`
  * `ArduinoJson`
1. Install the following by manually checking them out into your Arduino sketch directory:
  * [SonosUPnP](https://github.com/tmittet/sonos)
  * [microxpath](https://github.com/tmittet/microxpath)
1. Open `esp8266-sonos-plug.ino` included in this repository
  * Update the WiFi network and password appropriately
  * Update the IP/HW addresses as needed for your TP-Link plug and Sonos
1. Compile the sketch and upload it to your ESP8266
