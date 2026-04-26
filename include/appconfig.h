#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define APP_CONFIG_VERSION 1
#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 64
#define INFLUX_URL_MAX_LEN 256
#define INFLUX_USER_MAX_LEN 32
#define INFLUX_PASS_MAX_LEN 64

struct app_config {
  uint16_t tz_offset;
  char     influx_url[INFLUX_URL_MAX_LEN + 1];
  char     influx_user[INFLUX_USER_MAX_LEN + 1];
  char     influx_pass[INFLUX_PASS_MAX_LEN + 1];
};

extern struct app_config app_config;

/* API */
int appconfig_init(void);
int appconfig_save(void);
void appconfig_dump(void);
