#include <AppConfig.h>

#include <Preferences.h>
#include <esp_crc.h>

// config defaults
constexpr uint16_t defaultTz = 60; // 01:00 -- CET (winter time)
constexpr char default_rf24_channel = 210;
const uint8_t default_rf24_address[5] = {'a', 'b', 'b', 'a', '0'};
const char *default_influxdb_url = "http://homeassistant.local:8086/write?db=sensors";

AppConfig appConfig;

Preferences prefs;

static uint32_t cfgCrc(const struct AppConfig* c) {
  return esp_crc32_le(0, reinterpret_cast<const uint8_t*>(c), offsetof(AppConfig, crc));
}

bool AppConfig::checkCrc() {
    return cfgCrc(this) == crc;
}

void AppConfig::updateCrc() {
    crc = cfgCrc(this);
}

bool AppConfig::load() {
  prefs.begin("app", true);
  
  if (prefs.getBytesLength("cfg") != sizeof(AppConfig)) {
    prefs.end();
    rf24Channel = default_rf24_channel;
    memcpy(rf24Address, default_rf24_address, sizeof(rf24Address));    
    memcpy(influxUrl, default_influxdb_url, min(strlen(default_influxdb_url), INFLUX_URL_MAX_LEN));   
    timeZoneOffset = defaultTz;
    return false;
  }

  prefs.getBytes("cfg", this, sizeof(AppConfig));
  prefs.end();

  if (version != APP_CONFIG_VERSION) { 
    return false; 
  }

  if (!checkCrc()) { 
    return false; 
  }

  return true;
}

bool AppConfig::save() {
  version = APP_CONFIG_VERSION;
  updateCrc();

  prefs.begin("app", false);
  size_t w = prefs.putBytes("cfg", this, sizeof(AppConfig));
  prefs.end();
  if (w != sizeof(AppConfig)) { return false; }
  return true;
}

String formatTimezone(uint16_t offset) {
    char buf[13];
    sprintf(buf, "%s%02d:%02d", offset < 0 ? "-" : "", offset / 60, offset % 60);
    return String(buf);
}

void AppConfig::dump() {
  Serial.println(F("=== Config Dump ==="));
  
  Serial.print(F("Version:\t")); Serial.println(version);
  Serial.print(F("Reserved:\t")); Serial.println(reserved);
  Serial.print(F("WiFi SSID:\t")); Serial.println(ssid);
  Serial.print(F("Password:\t")); Serial.println(pass);  
  Serial.print(F("TimeZone:\t")); Serial.println(formatTimezone(timeZoneOffset));  
  Serial.print(F("RF24 Channel: ")); Serial.println(rf24Channel);
  Serial.print(F("RF24 Address: 0x"));
  for (int i = 0; i < 5; i++) Serial.printf("%02X", rf24Address[i]);
  Serial.println();
  Serial.print(F("Influx URL: ")); Serial.println(influxUrl);
  Serial.print(F("Influx User: ")); Serial.println(influxUser);
  Serial.print(F("Influx Password: ")); Serial.println(influxPass);

  // must be last
  Serial.print(F("CRC32:\t0x")); Serial.print(crc, HEX);
  if(checkCrc()) {
    Serial.println(F(" OK"));
  } else {
    Serial.println(F(" FAIL"));
  }

  Serial.println(F("==================="));
}