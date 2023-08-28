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

#define DURATION_MAX_LENGTH                  4    // enough for up to 9999

#define EEPROM_NUMBER_OF_BYTES_WE_USE        (EEPROM_WIFI_CONFIGURED_MAGIC_LENGTH + EEPROM_SSID_LENGTH + EEPROM_PASSWORD_LENGTH)

#define DEFAULT_SSID     "wifi_relay"

#define SHOW_CONFIG   12    // GPIO 12, D6 on silkscreen
#define RELAY_OUT     13    // GPIO 13, D7 on silkscreen
#define RESET_CONFIG  14    // GPIO 14, D5 on silkscreen

#define LCD_SCROLL_INTERVAL  3000     // How long a portion of a scrolling message will stay before scrolling to the next portion
#define LCD_TIMEOUT          5000     // LCD screen will turn off after this amount of time (when not in config mode)


ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 4);

bool wifiConfigured = false;
bool lcdIsOn = false;
unsigned long scrollTime = 0;
unsigned long lcdTimeoutCounter = 0;
bool relayOn = false;
unsigned long relayTimeoutCounter = 0;
unsigned long relayTimeout = 0;
bool ignoreRelayRequest = true;
String duration = "1";


typedef enum {
  HeaderMessage_None,
  HeaderMessage_BadInput,
  HeaderMessage_CurrentlyRunning
} HeaderMessage;

HeaderMessage controlFormHdrMessage = HeaderMessage_None;

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

String getRedirectHtml() {
  char  redirect_html[128] = {0}; //todo: lose the magic numbers
  char  ip_address[16] = {0};
  WiFi.localIP().toString().toCharArray(ip_address, 16);
  sprintf(redirect_html, "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"0; url='http://%s'\" /></head><body></body></html>", ip_address);
  String str = redirect_html;
  return str;
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

    server.send(200, "text/html", FPSTR(rebooting_html));
    // Too fast. I observe that when configuring from my phone, I don't get the rebooting response before it reboots.
    delay(500);
    ESP.restart();
  } else {
    out("Bad data!!\n");
    server.send(401);
  }

}


void sendControlForm() {
  // Build the html form to send
  String output_html = "";
  switch (controlFormHdrMessage) {
    case HeaderMessage_None:
      output_html = control_form_hdr;
      break;
    case HeaderMessage_BadInput:
      output_html = control_with_errors_form_hdr;
      break;
    case HeaderMessage_CurrentlyRunning:
      output_html = control_currently_running_form_hdr;
      break;
  }

  controlFormHdrMessage = HeaderMessage_None;
  char  action_html[512] = {0};  // todo: lose this magic number!
  char  ip_address[16] = {0};
  char  duration_str[DURATION_MAX_LENGTH + 1] = {0};
  WiFi.localIP().toString().toCharArray(ip_address, 16);
  duration.toCharArray(duration_str, DURATION_MAX_LENGTH + 1);

  sprintf(action_html, "<form action=\"http://%s/activateRelay\" method=\"GET\"><div><label for=\"duration\">How Long (seconds):</label><input name=\"duration\" id=\"duration\" value=\"%s\"/></div><div><button>GO</button></div></form><form action=\"http://%s/cancelRelay\" method=\"GET\"><button>STOP</button></form>", ip_address, duration_str, ip_address);
  output_html += action_html;

  server.send(200, "text/html", output_html);
  
  // Allow handleRelay to accept relay activation requests
  ignoreRelayRequest = false;
}

void handleRelay() {
  out("Received relay activation request\n");

  if (ignoreRelayRequest) {
    out("...rejected until control form get's loaded again.\n");
    // Redirect to control form
    server.send(200, "text/html", getRedirectHtml());
    return;
  }
  if (relayOn) {
    // Relay is currently on. Wait til it's done
    controlFormHdrMessage = HeaderMessage_CurrentlyRunning;
    server.send(200, "text/html", getRedirectHtml());
    return;
  }

  // Do not accept any more requests until the user reloads the control form.
  // This is to prevent an unintentional refresh of http://<ipaddr>/activateRelay from firing the relay.
  // Sometimes the user will refresh with an errant swipe down on their phone screen, or a browser will
  // refresh for god knows why, or just reloading a browser app will reload the page.
  // We need to be careful to ONLY activate the relay when the user clicks the form button.
  ignoreRelayRequest = true;

  if (server.args() == 1) {
    duration = server.arg(0);
    relayTimeout = (unsigned long)duration.toInt() * 1000;

    if (duration.length() > DURATION_MAX_LENGTH || relayTimeout == 0) {
      // No bueno. Signal bad input error and redirect to control form
      controlFormHdrMessage = HeaderMessage_BadInput;
      duration = "1";
      server.send(200, "text/html", getRedirectHtml());
      return;
    }

    // Signal to turn on the relay. The main loop() will do this.
    relayOn = true;

    out("Relay activation request for %d seconds\n", relayTimeout / 1000);
  } else {
    out("Bad data!!\n");
    server.send(401);
  }

  // Don't leave the user at this page
  server.send(200, "text/html", getRedirectHtml());

}


void handleCancel() {
  out("Received relay cancel request\n");
  if (relayOn) {
    relayTimeout = 0;
  }
  server.send(200, "text/html", getRedirectHtml());
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

  // don't leave this here. Move it to after wifi init.
  // I don't want the safety off the trigger until after wifi is configured. Here now just for testing.
  pinMode(RELAY_OUT, OUTPUT);
  digitalWrite(RELAY_OUT, LOW);

  EEPROM.begin(EEPROM_NUMBER_OF_BYTES_WE_USE);

  // This does not retun until it successfully connects to something
  connectToWifi();

  if (wifiConfigured) {
    // Wifi configured by user to connect to their router.
    // Setup handlers for prescribed device use.
    server.on("/", HTTP_GET, sendControlForm);
    server.on("/activateRelay", HTTP_GET, handleRelay);
    server.on("/cancelRelay", HTTP_GET, handleCancel);
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

    if (relayOn) {
      if (relayTimeoutCounter == 0) {
        // User requested relay on, but we haven't turned it on yet. So turn it on.
        out("Relay ON\n");
        relayTimeoutCounter = millis();
        digitalWrite(RELAY_OUT, HIGH);
      } else if (millis() - relayTimeoutCounter > relayTimeout) {
        // Relay time has elapsed. Turn it off.
        digitalWrite(RELAY_OUT, LOW);
        relayOn = false;
        relayTimeoutCounter = 0;
        out("Relay OFF\n");
      } else {
        // This simply means that the relay is currently ON, and the time for
        // it to stay on has not elapsed yet.
        // Nothing to do.
      }
    }

  }

}
















