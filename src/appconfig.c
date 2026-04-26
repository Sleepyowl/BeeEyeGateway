#include "appconfig.h"

#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_DECLARE(app);

struct app_config app_config;

static void appconfig_defaults(void) {
    memset(&app_config, 0, sizeof(app_config));

    app_config.tz_offset = 0;
}

static int appconfig_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(key, "tz_offset") == 0) {
        read_cb(cb_arg, &app_config.tz_offset, sizeof(int));
    } else if (strcmp(key, "influx_url") == 0) {
        read_cb(cb_arg, app_config.influx_url, sizeof(app_config.influx_url));
    } else if (strcmp(key, "influx_user") == 0) {
        read_cb(cb_arg, app_config.influx_user, sizeof(app_config.influx_user));
    } else if (strcmp(key, "influx_pass") == 0) {
        read_cb(cb_arg, app_config.influx_pass, sizeof(app_config.influx_pass));
    } else {
        return -ENOENT;
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(appconfig, "app", NULL, appconfig_set, NULL, NULL);


int appconfig_save(void)
{
    int rc = 0;

    rc |= settings_save_one("app/tz_offset", &app_config.tz_offset, sizeof(int));
    rc |= settings_save_one("app/influx_url", app_config.influx_url, strlen(app_config.influx_url) + 1);
    rc |= settings_save_one("app/influx_user", app_config.influx_user, strlen(app_config.influx_user) + 1);
    rc |= settings_save_one("app/influx_pass", app_config.influx_pass, strlen(app_config.influx_pass) + 1);

    return rc;
}

int appconfig_init(void)
{
    int rc;

    appconfig_defaults();

    rc = settings_subsys_init();
    if (rc) {
        LOG_ERR("settings init failed (%d)", rc);
        return rc;
    }

    rc = settings_load();
    if (rc) {
        LOG_ERR("settings load failed (%d)", rc);
        return rc;
    }

    LOG_INF("app config loaded");
    return 0;
}

void appconfig_dump(void)
{
    char buf[9];
    uint16_t offset = app_config.tz_offset;
    snprintf(buf, sizeof(buf), "%s%02d:%02d", (offset < 0 ? "-" : ""), offset / 60, offset % 60);

    LOG_INF("=== AppConfig ===");
    LOG_INF("Time zone   : %s", buf);
    LOG_INF("Influx URL  : %s", app_config.influx_url);
    LOG_INF("Influx user : %s", app_config.influx_user);
    LOG_INF("Influx pass : %s", app_config.influx_pass);
}