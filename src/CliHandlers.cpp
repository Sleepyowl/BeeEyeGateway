#include "LedStatus.h"
#include "AppConfig.h"
#include "WiFiUtil.h"

#include <Arduino.h>
#include <WiFi.h>

void cmdStatus(const char*) {
  Serial.println("Unit status:");

  Serial.print("WiFi:\t");
  if(WiFi.status() == WL_CONNECTED) {
    Serial.printf("CONNECTED to %s (%ld dBm)\n", WiFi.SSID().c_str(), WiFi.RSSI());
  } else {
    Serial.println("NOT CONNECTED");
  }

  Serial.flush();
  delay(300);
}

void cmdRestart(const char*) {
  ESP.restart();
}

void cmdConnect(const char*) {
  setPixelSignal(PixelSignal::Initializing);
  if(connectWifi()) {
    Serial.printf("Connected to network %s\n", appConfig.ssid);    
    setPixelSignal(PixelSignal::Ok);
    delay(500);
    setPixelSignal(PixelSignal::None);
  } else {
    Serial.println("Couldn't connect to WiFi");
  }
}

void cmdScanWifi(const char* arg) {
  scanWifiNetworks();
}

void cmdSave(const char*) {  
  if(appConfig.save()) {
    Serial.println("Config saved");
  } else {
    Serial.println("Couldn't save config");
  }
}

void cmdDump(const char*) {
    appConfig.dump();
}

void cmdSetSsid(const char* ssid) {
  Serial.printf("Setting WiFi SSID to %s\n", ssid);
  const size_t n = strlen(ssid);
  if (n > SSID_MAX_LEN) { Serial.println("SSID too long (max 32)"); return; }
  strncpy(appConfig.ssid, ssid, n);
  appConfig.ssid[n] = '\0';
}

void cmdSetPass(const char* pass) {
  Serial.printf("Setting WiFi password to %s\n", pass);
  const size_t n = strlen(pass);
  if (n < 8 || n > 63) { Serial.println("Password 8–63"); return; }
  strncpy(appConfig.pass, pass, n);
  appConfig.pass[n] = '\0';
}

void cmdSetTz(const char* s) {
  Serial.printf("Setting TZ to %s\n", s);
  if (!s || !(*s)) {
    appConfig.timeZoneOffset = 0;  
    return;
  }
  int sign = (*s == '-') ? -1 : 1;
  if (*s == '+' || *s == '-') s++;
  int h = 0, m = 0;
  sscanf(s, "%d:%d", &h, &m);
  const auto offset =  sign * (h * 60 + m);
  appConfig.timeZoneOffset = offset;
  Serial.printf("Offset in minutes = %d\n", offset);
}

void cmdSetInfluxUrl(const char* url) {
  const size_t n = strlen(url);
  if (n < 8 || n > 256) { Serial.println("Influx URL 8–256"); return; }
  strncpy(appConfig.influxUrl, url, n);
  appConfig.influxUrl[n] = '\0';
  Serial.printf("InfluxDB URL = %s\n", url);
}

void cmdSetInfluxUser(const char* user) {
  const size_t n = strlen(user);
  if (n > INFLUX_USER_MAX_LEN) { Serial.println("Influx User too long"); return; }
  strncpy(appConfig.influxUser, user, n);
  appConfig.influxUser[n] = '\0';
  Serial.printf("InfluxDB User = %s\n", user);
}

void cmdSetInfluxPass(const char* pass) {
  const size_t n = strlen(pass);
  if (n > INFLUX_PASS_MAX_LEN) { Serial.println("Influx Password too long"); return; }
  strncpy(appConfig.influxPass, pass, n);
  appConfig.influxPass[n] = '\0';
  Serial.printf("InfluxDB Pass = %s\n", pass);
}

void cmdSetRf24Channel(const char* s) {
  constexpr int maxch = 125;
  int ch = 0;
  sscanf(s, "%d", &ch);
  ch = min(ch, maxch);
  appConfig.rf24Channel = ch;
  Serial.printf("RF24 Channel = %d\n", ch);
}