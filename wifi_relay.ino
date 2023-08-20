



#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "html_strings.h"


#define out(...) Serial.printf(__VA_ARGS__)
#define outl  Serial.println()


#define EEPROM_WIFI_CONFIGURED_BYTE      0    // 0 = wifi NOT configured, 1 = wifi configured
#define EEPROM_SSID_OFFSET               1
#define EEPROM_SSID_LENGTH               32   // max length for ssid, no null (according to the googles)
#define EEPROM_PASSWORD_OFFSET           33
#define EEPROM_PASSWORD_LENGTH           63   // max length for password, no null (according to the googles)

#define EEPROM_NUMBER_OF_BYTES_WE_USE    (1 + EEPROM_SSID_LENGTH + EEPROM_PASSWORD_LENGTH)

#define DEFAULT_SSID     "wifi_relay"


ESP8266WebServer server(80);

String get_eeprom_string(int offset, int length) {
  String str = "";
  for (int i = offset; i < offset + length; i++) {
    str += EEPROM.read(i);
  }
  return str + '\0';
}

void handleSetup() {
  out("Received wifi config data\n");

  String ssid = "";
  String password = "";

  if (server.args() == 2) {
    ssid = server.arg(0);
    password = server.arg(1);

    out("SSID: %s\n", ssid);
    out("Pass: %s\n", password);

    server.send(200);
  } else {
    out("nae data!!\n");
    server.send(401);
  }


}


void setup() {
  delay(3000);

  Serial.begin(115200);
  out("startup\n");

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  char wifi_configured = EEPROM.read(EEPROM_WIFI_CONFIGURED_BYTE);
  //EEPROM.write(EEPROM_WIFI_CONFIGURED_BYTE, (byte)notificationsMuted);
  //EEPROM.commit();
  out("mystery char: %c\n", wifi_configured);


  if (wifi_configured) {
    String ssid = get_eeprom_string(EEPROM_SSID_OFFSET, EEPROM_SSID_LENGTH);
    String password = get_eeprom_string(EEPROM_PASSWORD_OFFSET, EEPROM_PASSWORD_LENGTH);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    out("Connecting to %s", ssid);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      out(".");
    }
    out("connected.\n");
  } else {
    out("No ssid configured\n");
    out("Entering AP mode. SSID: %s\n", DEFAULT_SSID);
    WiFi.softAP(DEFAULT_SSID);
    out("APIP: ");
    Serial.println(WiFi.softAPIP());
    server.on("/", HTTP_GET, handleSetup);
    server.begin();
  }

}

void loop() {
  server.handleClient();
}


































