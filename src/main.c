#include "display.h"
#include "appconfig.h"
#include "wifi.h"
#include "lora.h"
#include "rtc_clock.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/random/random.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/sys/reboot.h>

#include "esp_system.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static int post(void);

static const struct device *dev_display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static const char *get_reset_reason_string(esp_reset_reason_t reset_reason);

int main(void)
{
    int ret;
    k_msleep(1000); // so tio has time to connect

    esp_reset_reason_t rst_reason = esp_reset_reason();
    LOG_INF("RESET REASON: %s (%d)", get_reset_reason_string(rst_reason), rst_reason);

    if (!device_is_ready(dev_display)) {
        LOG_ERR("Display is not ready");
        return -ENODEV;
    } else {
        display_blanking_off(dev_display);
        display_start();
    }

    ret = post();
    if (ret) {
        LOG_ERR("POST failed");
        return 0;
    }

    display_printf("Connecting to WiFi... ");
    ret = wifi_auto_connect();
    if (ret == -ENOENT) {
        display_printf("FAILED.\nNo WiFi credentials stored.\n");
    } else if (ret == -ENETUNREACH) {
        display_printf("FAILED.\nCouldn't connect to any network.\n");
    } else if (ret) {
        display_printf("FAILED.\nError code: %d\n", ret);
    } else {
        display_printf("OK.\n");
    }

    ret = lora_start();
    if (ret) {
        LOG_ERR("LoRa start failed: %d", ret);
        return 0;
    }

    display_printf("Ready\n");

    uint64_t last_sync = 0;
    while (1) {
        k_msleep(30000);

        uint64_t now = k_uptime_get();
        if (wifi_is_connected() && (last_sync == 0 || (now - last_sync) / 3600000 > 24)) {
            rtc_synchronize_time();
            last_sync = now;
        }

        if (!wifi_is_connected()) {
            ret = wifi_auto_connect();
            if(!ret) {
                display_printf("WiFi connected\n");
            }
        }
    }

    return 0;
}

static const struct device *dev_rtc = DEVICE_DT_GET(DT_NODELABEL(rv3028));
static const struct device *dev_lora = DEVICE_DT_GET(DT_NODELABEL(lora));

static int post(void) {
    int ret;

    if (!device_is_ready(dev_rtc)) {
        LOG_ERR("RTC not ready");
        return -ENODEV;
    }

    if (!device_is_ready(dev_lora)) {
        LOG_ERR("LoRa not ready");
        return -ENODEV;
    }

    ret = appconfig_init();
    if (ret) {
        LOG_ERR("Couldn't load app settings: %d", ret);
    }

    ret = task_wdt_init(NULL);
    if (ret) {
        LOG_ERR("Couldn't init Task WDT: %d", ret);
    }

    ret = wifi_init();
    if (ret) {
        LOG_ERR("WiFi init failed: %d", ret);
    }

    return 0;
}

static const char *get_reset_reason_string(esp_reset_reason_t reset_reason)
{
    switch (reset_reason) {
    case ESP_RST_UNKNOWN:    return "UNKNOWN";
    case ESP_RST_POWERON:    return "POWERON";
    case ESP_RST_EXT:        return "EXTERNAL";
    case ESP_RST_SW:         return "SW";
    case ESP_RST_PANIC:      return "PANIC";
    case ESP_RST_INT_WDT:    return "INT_WDT";
    case ESP_RST_TASK_WDT:   return "TASK_WDT";
    case ESP_RST_WDT:        return "WDT";
    case ESP_RST_DEEPSLEEP:  return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:   return "BROWNOUT";
    case ESP_RST_SDIO:       return "SDIO";
    case ESP_RST_USB:        return "USB";
    case ESP_RST_JTAG:       return "JTAG";
    case ESP_RST_EFUSE:      return "EFUSE";
    case ESP_RST_PWR_GLITCH: return "PWR_GLITCH";
    case ESP_RST_CPU_LOCKUP: return "CPU_LOCKUP";
    default:                 return "INVALID";
    }
}