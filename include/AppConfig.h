#pragma once
#include <stdint.h>
#include <stddef.h>

constexpr uint16_t APP_CONFIG_VERSION = 1;
constexpr size_t   SSID_MAX_LEN   = 32;
constexpr size_t   PASS_MAX_LEN   = 64;
constexpr size_t   INFLUX_URL_MAX_LEN   = 256;
constexpr size_t   INFLUX_USER_MAX_LEN   = 32;
constexpr size_t   INFLUX_PASS_MAX_LEN   = 64;

struct AppConfig final {
  uint16_t version;
  uint16_t reserved;
  char     ssid[SSID_MAX_LEN + 1];
  char     pass[PASS_MAX_LEN + 1];
  uint16_t timeZoneOffset;
  uint8_t  rf24Channel;
  uint8_t  rf24Address[5];
  char     influxUrl[INFLUX_URL_MAX_LEN + 1];
  char     influxUser[INFLUX_USER_MAX_LEN + 1];
  char     influxPass[INFLUX_PASS_MAX_LEN + 1];

  // Must be last
  uint32_t crc;     // CRC32 over everything before this field

  bool checkCrc();
  void updateCrc();
  bool load();
  bool save();
  void dump();
};

static_assert(sizeof(AppConfig) - sizeof(AppConfig::crc) == offsetof(AppConfig, crc));

extern AppConfig appConfig;
