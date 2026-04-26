#include "appconfig.h"
#include "wifi.h"

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/wifi_mgmt.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int cmd_restart(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Rebooting...");
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

static int cmd_save(const struct shell *sh, size_t argc, char **argv)
{
    int ret;
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    ret = appconfig_save();
    if (ret) {
        shell_error(sh, "Couldn't save config: %d", ret);
    } else {
        shell_print(sh, "Config saved");        
    }

    return 0;
}

static int cmd_dump(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    char buf[9];
    uint16_t offset = app_config.tz_offset;
    snprintf(buf, sizeof(buf), "%s%02d:%02d", (offset < 0 ? "-" : ""), offset / 60, offset % 60);

    shell_print(sh, "=== AppConfig ===");

    shell_print(sh, "Timezone       : %s", app_config.tz_offset ? buf : "UTC");
    shell_print(sh, "Influx URL     : %s", app_config.influx_url[0] ? app_config.influx_url : "<empty>");
    shell_print(sh, "Influx User    : %s", app_config.influx_user[0] ? app_config.influx_user : "<empty>");
    shell_print(sh, "Influx Pass    : %s", app_config.influx_pass[0] ? app_config.influx_pass : "<empty>");

    return 0;
}

static int cmd_timezone(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: timezone +-HH:MM");
        return -EINVAL;
    }

    const char *s = argv[1];
    int sign = (*s == '-') ? -1 : 1;

    if (*s == '+' || *s == '-') {
        s++;
    }

    int h = 0, m = 0;
    sscanf(s, "%d:%d", &h, &m);

    int offset = sign * (h * 60 + m);
    app_config.tz_offset = offset;

    shell_print(sh, "Timezone offset = %d minutes", offset);
    return 0;
}

static int cmd_influxurl(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: influxurl <url>");
        return -EINVAL;
    }

    const char *url = argv[1];
    size_t n = strlen(url);

    if (n < 8 || n > 256) {
        shell_error(sh, "URL length 8–256");
        return -EINVAL;
    }

    strncpy(app_config.influx_url, url, n);
    app_config.influx_url[n] = '\0';

    shell_print(sh, "Influx URL set");
    return 0;
}

static int cmd_influxuser(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: influxuser <user>");
        return -EINVAL;
    }

    const char *user = argv[1];
    size_t n = strlen(user);

    if (n > INFLUX_USER_MAX_LEN) {
        shell_error(sh, "User too long");
        return -EINVAL;
    }

    strncpy(app_config.influx_user, user, n);
    app_config.influx_user[n] = '\0';

    shell_print(sh, "Influx user set");
    return 0;
}

static int cmd_influxpass(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: influxpass <pass>");
        return -EINVAL;
    }

    const char *pass = argv[1];
    size_t n = strlen(pass);

    if (n > INFLUX_PASS_MAX_LEN) {
        shell_error(sh, "Password too long");
        return -EINVAL;
    }

    strncpy(app_config.influx_pass, pass, n);
    app_config.influx_pass[n] = '\0';

    shell_print(sh, "Influx password set");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mgr_cmds,
    SHELL_CMD(save,      NULL, "Save config", cmd_save),
    SHELL_CMD(dump,      NULL, "Dump config", cmd_dump),
    SHELL_CMD(restart,   NULL, "Reboot device", cmd_restart),
    SHELL_CMD(timezone,  NULL, "Set timezone +-HH:MM", cmd_timezone),
    SHELL_CMD(influxurl, NULL, "Set InfluxDB URL", cmd_influxurl),
    SHELL_CMD(influxuser,NULL, "Set InfluxDB user", cmd_influxuser),
    SHELL_CMD(influxpass,NULL, "Set InfluxDB password", cmd_influxpass),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mgr, &mgr_cmds, "Device Management commands", NULL);