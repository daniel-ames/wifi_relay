



#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "html_strings.h"


#define out(...) Serial.printf(__VA_ARGS__)


#define EEPROM_WIFI_CONFIGURED_BYTE  0    // 0 = wifi NOT configured, 1 = wifi configured
#define EEPROM_SSID                  1    // ssid string, no null
#define EEPROM_PASSWORD              33   // password string, no null
#define EEPROM_NUMBER_OF_BYTES_WE_USE  (1 + 32 + 63)  // 1 for the wifi_configured byte, 32 for ssid max length of 32, 63 for password max length of 63. (According to the googles)


ESP8266WebServer server(80);

String get_eeprom_string(int offset, int length) {
  String str = "";
  for (int i = offset; i < offset + length; i++) {
    str += EEPROM.read(i);
  }
  return str + '\0';
}


void setup() {
  delay(3000);

  Serial.begin(115200);
  out("startup\n");

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  char wifi_configured = EEPROM.read(EEPROM_WIFI_CONFIGURED_BYTE);
  //EEPROM.write(EEPROM_WIFI_CONFIGURED_BYTE, (byte)notificationsMuted);
  //EEPROM.commit();


  if (wifi_configured) {
    String ssid = get_eeprom_string();
    String password = EEPROM.read(10);;
    WiFi.mode(WIFI_STA);

  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
