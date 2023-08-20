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

#define RESET_CONFIG  5    // GPIO 5, D1 on silkscreen


ESP8266WebServer server(80);

short pushButtonSemaphore = 0;
unsigned long lastInterruptTime = 0;
unsigned long interruptTime;
void IRAM_ATTR PbVector() {
  // Hit the semaphore ONLY within the bounds of the debounce timer.
  // In other words, the only piece of code in here that's not debounce
  // logic is "pushButtonSemaphore++"
  // I learned the hard way that ISR's must be blazingly quick on ESP32,
  // else the watchdog will think the loop() is in the weeds and will reboot
  // the damn cpu.
  interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) pushButtonSemaphore++;
  lastInterruptTime = interruptTime;
}


void get_eeprom_buffer(int offset, int length, char *buf) {
  int buf_offset = 0;
  for (int i = offset; i < offset + length; i++) {
    buf[buf_offset++] = EEPROM.read(i);
  }
}

bool write_eeprom_buffer(int offset, int length, char *buf) {
  int buf_offset = 0;
  for (int i = offset; i < offset + length; i++) {
    EEPROM.write(i, buf[buf_offset++]);
  }
  return EEPROM.commit();
}

void handleSetup() {
  out("Received wifi config data\n");

  String ssid = "";
  String password = "";

  char ssidBuf[EEPROM_SSID_LENGTH + 1] = {0};
  char passBuf[EEPROM_PASSWORD_LENGTH + 1] = {0};

  if (server.args() == 2) {
    ssid = server.arg(0);
    password = server.arg(1);

    if (ssid.length() > EEPROM_SSID_LENGTH || password.length() > EEPROM_PASSWORD_LENGTH) {
      // No bueno
      server.send(200, "text/html", FPSTR(config_with_errors_form));
      return;
    }

    ssid.toCharArray(ssidBuf, EEPROM_SSID_LENGTH);
    password.toCharArray(passBuf, EEPROM_PASSWORD_LENGTH);

    out("SSID: %s\n", ssidBuf);
    out("Pass: %s\n", passBuf);

    // Commit settings to EEPROM
    if (write_eeprom_buffer(EEPROM_SSID_OFFSET, EEPROM_SSID_LENGTH, ssidBuf) &&
        write_eeprom_buffer(EEPROM_PASSWORD_OFFSET, EEPROM_PASSWORD_LENGTH, passBuf)) {
      // Don't write the magic unless the actual wifi config data wrote successfully
      write_eeprom_buffer(EEPROM_WIFI_CONFIGURED_MAGIC_OFFSET, EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH, MAGIC);
    } else {
      out("\nFAILED!!\n");
    }

    server.send(200, "text/html", FPSTR(rebooting));
    // Too fast. I observe that when configuring from my phone, I don't get the rebooting response before it reboots.
    delay(500);
    ESP.restart();
  } else {
    out("Bad data!!\n");
    server.send(401);
  }

}

bool WifiConfigured() {
  char sig[9];
  sig[8] = 0;

  get_eeprom_buffer(EEPROM_WIFI_CONFIGURED_MAGIC_OFFSET, EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH, sig);

  for (int i = 0; i < EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH; i++)
    if (MAGIC[i] != sig[i]) return false;

  return true;
}

bool wipe_config_interrupt_in_service = false;
void IRAM_ATTR WipeConfig() {

  // debounce - just in case this function fails to reset the cpu for some reason.
  if (wipe_config_interrupt_in_service) return;
  wipe_config_interrupt_in_service = true;

  out("\n\nWiping EEPROM config space");
  for (int i = 0; i < EEPROM_NUMBER_OF_BYTES_WE_USE; i++) {
    EEPROM.write(i, 0);
    Serial.print(".");
  }
  Serial.println();
  if (EEPROM.commit()) {
    Serial.println("EEPROM config space successfully wiped.");
    ESP.restart();
  } else {
    Serial.println("ERROR! EEPROM config space wipe failed!");
  }
  wipe_config_interrupt_in_service = false;
}

void setup() {
  delay(3000);

  Serial.begin(115200);
  out("startup\n");

  pinMode(RESET_CONFIG, INPUT_PULLUP);
  attachInterrupt(RESET_CONFIG, WipeConfig, FALLING);

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  //EEPROM.write(EEPROM_WIFI_CONFIGURED_BYTE, (byte)notificationsMuted);
  //EEPROM.commit();

  if (WifiConfigured()) {
    char ssid[33] = {0};
    char password[64] = {0};
    get_eeprom_buffer(EEPROM_SSID_OFFSET, EEPROM_SSID_LENGTH, ssid);
    get_eeprom_buffer(EEPROM_PASSWORD_OFFSET, EEPROM_PASSWORD_LENGTH, password);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    out("Connecting to %s", ssid);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      out(".");
    }
    out("connected at ");
    Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", FPSTR(welcome_form));
    });
  } else {
    out("No ssid configured\n");
    out("Entering AP mode. SSID: %s\n", DEFAULT_SSID);
    WiFi.softAP(DEFAULT_SSID);
    out("APIP: ");
    Serial.println(WiFi.softAPIP());
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", FPSTR(config_form));
    });
    server.on("/setConfig", HTTP_GET, handleSetup);
  }
  server.begin();

}

void loop() {
  server.handleClient();

}
















