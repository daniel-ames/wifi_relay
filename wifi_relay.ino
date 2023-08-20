



#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "html_strings.h"


#define out(...) Serial.printf(__VA_ARGS__)
#define outl  Serial.println()


char MAGIC[] = {'W', 'i', 'F', 'i', 'C', 'O', 'N', 'F'};

#define EEPROM_WIFI_CONFIGURED_MAGIC_OFFSET  0    // MAGC = wifi configured, anything else = wifi NOT configured
#define EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH  8
#define EEPROM_SSID_OFFSET                   8
#define EEPROM_SSID_LENGTH                   32   // max length for ssid, no null (according to the googles)
#define EEPROM_PASSWORD_OFFSET               40
#define EEPROM_PASSWORD_LENGTH               63   // max length for password, no null (according to the googles)

#define EEPROM_NUMBER_OF_BYTES_WE_USE        (EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH + EEPROM_SSID_LENGTH + EEPROM_PASSWORD_LENGTH)

#define DEFAULT_SSID     "wifi_relay"


ESP8266WebServer server(80);

void get_eeprom_buffer(int offset, int length, char *buf) {
  for (int i = offset; i < offset + length; i++) {
    buf[i] = EEPROM.read(i);
  }
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

bool WifiConfigured() {
  char sig[9];
  sig[8] = 0;

  get_eeprom_buffer(EEPROM_WIFI_CONFIGURED_MAGIC_OFFSET, EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH, sig);

  out("sig: %s\n", sig);

  for (int i = 0; i < EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH; i++) {
    out("%d: %c", i, MAGIC[i]);
    out("    %d: %c\n", i, sig[i]);
    if (MAGIC[i] != sig[i]) return false;
  }

  return true;
}

void setup() {
  delay(3000);

  Serial.begin(115200);
  out("startup\n");

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  //EEPROM.write(EEPROM_WIFI_CONFIGURED_BYTE, (byte)notificationsMuted);
  //EEPROM.commit();

  if (WifiConfigured()) {
    // String ssid = get_eeprom_buffer(EEPROM_SSID_OFFSET, EEPROM_SSID_LENGTH);
    // String password = get_eeprom_buffer(EEPROM_PASSWORD_OFFSET, EEPROM_PASSWORD_LENGTH);
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);
    // out("Connecting to %s", ssid);
    // while (WiFi.status() != WL_CONNECTED) {
    //   delay(500);
    //   out(".");
    // }
    // out("connected.\n");
    out("configued!\n");
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


































