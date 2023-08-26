#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "html_strings.h"


#define out(...) Serial.printf(__VA_ARGS__)
#define outl  Serial.println()


char MAGIC[] = {'W', 'i', 'F', 'i', 'C', 'O', 'N', 'F'};

#define EEPROM_WIFI_CONFIGURED_MAGIC_OFFSET  0    // WiFiCONF = wifi configured, anything else = wifi NOT configured
#define EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH  8
#define EEPROM_SSID_OFFSET                   8
#define EEPROM_SSID_LENGTH                   32   // max length for ssid, no null (according to the googles)
#define EEPROM_PASSWORD_OFFSET               40
#define EEPROM_PASSWORD_LENGTH               63   // max length for password, no null (according to the googles)

#define EEPROM_NUMBER_OF_BYTES_WE_USE        (EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH + EEPROM_SSID_LENGTH + EEPROM_PASSWORD_LENGTH)

#define DEFAULT_SSID     "wifi_relay"

#define RESET_CONFIG  14    // GPIO 14, D5 on silkscreen
#define SHOW_CONFIG   12    // GPIO 12, D6 on silkscreen

#define LCD_SCROLL_INTERVAL  3000     // How long a portion of a scrolling message will stay before scrolling to the next portion
#define LCD_TIMEOUT          5000     // LCD screen will turn off after this amount of time (when not in config mode)


ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 4);

bool wifiConfigured = false;
bool lcdIsOn = false;
unsigned long scrollTime = 0;
unsigned long lcdTimeoutCounter = 0;

typedef enum {
  ToConfigureMe,
  ConnectToSsid,
  ThenGoTo
} ScrollPortion;

ScrollPortion scrollPortion = ThenGoTo;


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


void connectToWifi()
{
  int dotLocation = 13;
  int dots = 0;
  int wifiRetries = 0;
  if (!lcdIsOn) {
    lcd.display();
    lcd.backlight();
    lcdIsOn = true;
  }
  lcd.clear();
  out("WiFi is down. Connecting");

  wifiConfigured = WifiConfigured();
  if (wifiConfigured) {
    char ssid[33] = {0};
    char password[64] = {0};
    get_eeprom_buffer(EEPROM_SSID_OFFSET, EEPROM_SSID_LENGTH, ssid);
    get_eeprom_buffer(EEPROM_PASSWORD_OFFSET, EEPROM_PASSWORD_LENGTH, password);
    lcd.setCursor(0,0);
    lcd.print("Connecting to");
    lcd.setCursor(0,1);
    lcd.print(ssid);
    out("Connecting to %s", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      out(".");
      if (dots == 0) {
        lcd.setCursor(dotLocation,0);
        lcd.print(".  ");
        dots++;
      } else if (dots == 1) {
        lcd.setCursor(dotLocation,0);
        lcd.print(".. ");
        dots++;
      } else if (dots == 2) {
        lcd.setCursor(dotLocation,0);
        lcd.print("...");
        dots = 0;
      }
    }

    // connected
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SSID:");
    lcd.print(ssid);
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
    out("connected at ");
    Serial.println(WiFi.localIP());

  } else {
    out("\nNo ssid configured\n");
    out("Entering AP mode. SSID: %s\n", DEFAULT_SSID);
    WiFi.softAP(DEFAULT_SSID);
    out("APIP: ");
    Serial.println(WiFi.softAPIP());
  }

}



void setup() {
  delay(3000);

  Serial.begin(115200);
  out("startup\n");

  lcd.init();

  pinMode(RESET_CONFIG, INPUT_PULLUP);
  attachInterrupt(RESET_CONFIG, WipeConfig, FALLING);

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  // This does not retun until it successfully connects to something
  connectToWifi();

  if (wifiConfigured) {
    // Wifi configured by user to connect to their router.
    // Setup handlers for prescribed device use.
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", FPSTR(welcome_form));
    });
  } else {
    // Wifi not configured by user. Setup handlers for wifi configuration.
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", FPSTR(config_form));
    });
    server.on("/setConfig", HTTP_GET, handleSetup);
  }

  // Set the push button for GPIO12, and attch to PbVector()
  pinMode(SHOW_CONFIG, INPUT_PULLUP);
  attachInterrupt(SHOW_CONFIG, PbVector, FALLING);

  server.begin();

}


void loop() {
  server.handleClient();
  
  if (!wifiConfigured) {
    if (scrollTime == 0) {
      scrollTime = millis();
    } else if (millis() - scrollTime > LCD_SCROLL_INTERVAL) {
      if(scrollPortion == ThenGoTo) {
        // show the first portion
        //    To configure me,
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("To configure me,");
        scrollPortion = ToConfigureMe;
      } else if(scrollPortion == ToConfigureMe) {
        // show the second portion
        //    connect to:
        //    "wifi_relay"
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("connect to:");
        lcd.setCursor(0,1);
        lcd.print("\"");
        lcd.print(DEFAULT_SSID);
        lcd.print("\"");
        scrollPortion = ConnectToSsid;
      } else if(scrollPortion == ConnectToSsid) {
        // show the third portion
        //    Then browse to:
        //    192.168.4.1
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Then browse to:");
        lcd.setCursor(0,1);
        lcd.print(WiFi.softAPIP());
        scrollPortion = ThenGoTo;
      }

      // reset the timer
      scrollTime = millis();
    }
  }

  if (wifiConfigured) {

    // Turn off the lcd if it's been on for too long
    if (lcdIsOn) {
      if (lcdTimeoutCounter == 0) {
        lcdTimeoutCounter = millis();
      } else if (millis() - lcdTimeoutCounter > LCD_TIMEOUT) {
        lcdIsOn = false;
        lcd.noDisplay();
        lcd.noBacklight();
      }
    }

    if (pushButtonSemaphore) {
      if (!lcdIsOn) {
        lcd.display();
        lcd.backlight();
      }
      lcdIsOn = true;
      lcdTimeoutCounter = 0;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("SSID:");
      lcd.print(WiFi.SSID());
      lcd.setCursor(0,1);
      lcd.print(WiFi.localIP());

      pushButtonSemaphore = 0;
    }

  }

}
















