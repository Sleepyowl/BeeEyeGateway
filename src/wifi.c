#include "wifi.h"
#include "display.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_credentials.h>

LOG_MODULE_DECLARE(app);

static volatile int wifi_connected = 0;

bool wifi_is_connected(void) {
    return wifi_connected;
}



static struct net_mgmt_event_callback wifi_cb;
static K_SEM_DEFINE(wifi_connect_done, 0, 1);
static int last_wifi_connect_result;
struct wifi_connect_req_params wifi_connect_params = {
    .security = WIFI_SECURITY_TYPE_PSK,
    .channel = WIFI_CHANNEL_ANY
};

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;

        if (status->status == WIFI_STATUS_CONN_SUCCESS) {
            LOG_INF("WiFi connected");
            k_sem_give(&wifi_connect_done);
            wifi_connected = 1;
            last_wifi_connect_result = 0;
        } else {
            LOG_ERR("WiFi connect failed (%d)", status->status);
            k_sem_give(&wifi_connect_done);
            last_wifi_connect_result = -ECONNREFUSED;
        }
    }

    if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        LOG_WRN("WiFi disconnected");        
        if (!wifi_connected) {
            last_wifi_connect_result = -ECONNREFUSED;
            k_sem_give(&wifi_connect_done);
        } else {
            display_printf("WiFi disconnected\n");
            wifi_connected = 0;
        }
    }
}

int wifi_init(void) {
    net_mgmt_init_event_callback(
        &wifi_cb,
        wifi_event_handler,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT
    );
    net_mgmt_add_event_callback(&wifi_cb);
  return 0;
}

int wifi_connect(const char *ssid)
{
    int ret;
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default interface");
        return -ENODEV;
    }

    if (ssid == NULL || strlen(ssid) > 32) {
        LOG_ERR("Invalid ssid");
        return -EINVAL;
    }

    wifi_connect_params.ssid = ssid;
    wifi_connect_params.ssid_length = strlen(ssid);

    struct wifi_credentials_personal creds;
    ret = wifi_credentials_get_by_ssid_personal_struct(
                wifi_connect_params.ssid, wifi_connect_params.ssid_length, &creds);

    if (ret) {
        return ret;
    }

    wifi_connect_params.psk = creds.password;
    wifi_connect_params.psk_length = creds.password_len;

    LOG_INF("Connecting to %s", ssid);

    wifi_connected = 0;
    ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_connect_params, sizeof(wifi_connect_params));

    if (ret) {
        LOG_ERR("Connect request failed: %d", ret);
        return ret;
    }

    /* Wait for result */
    if (k_sem_take(&wifi_connect_done, K_SECONDS(45)) == 0) {
        return last_wifi_connect_result;
    }

    return -ETIMEDOUT;
}


size_t saved_ssids_count = 0;
#define MAX_SAVED_SSID_COUNT 20
char saved_ssids[MAX_SAVED_SSID_COUNT][33];
int saved_rssi[MAX_SAVED_SSID_COUNT];
K_SEM_DEFINE(wifi_filtered_scan_done, 0, 1);

static struct net_mgmt_event_callback wifi_filtered_scan_cb;

void wifi_credentials_ssid_handler(void *cb_arg, const char *ssid, size_t ssid_len)
{
    if (saved_ssids_count >= MAX_SAVED_SSID_COUNT) {
        LOG_WRN("Ignoring %s: too many saved networks", ssid);
        return;
    }

    if (ssid_len > 0 && ssid_len < 33) {
        memcpy(saved_ssids[saved_ssids_count], ssid, ssid_len);
        saved_ssids[saved_ssids_count][ssid_len] = '\0';
        saved_rssi[saved_ssids_count] = INT_MIN;
        saved_ssids_count++;
    } else {
        LOG_WRN("Invalid SSID length: %u", (unsigned int)ssid_len);
    }
}

static void filtered_scan_handler(struct net_mgmt_event_callback *cb,
                                  uint64_t mgmt_event,
                                  struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *res = cb->info;

        for (size_t i = 0; i < saved_ssids_count; i++) {
            if (strncmp((const char *)saved_ssids[i],
                        res->ssid,
                        res->ssid_length) == 0 &&
                strlen((const char *)saved_ssids[i]) == res->ssid_length) {

                saved_rssi[i] = res->rssi;
                LOG_DBG("SSID %s RSSI %d", saved_ssids[i], res->rssi);
            }
        }
    }

    if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
        k_sem_give(&wifi_filtered_scan_done);
    }
}

int wifi_scan_filtered(void) {
    struct net_if *iface = net_if_get_default();

    struct wifi_scan_params params = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    for (size_t i = 0; i < saved_ssids_count; i++) {
        saved_rssi[i] = INT_MIN;
    }

    net_mgmt_init_event_callback(&wifi_filtered_scan_cb,
                                 filtered_scan_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT |
                                 NET_EVENT_WIFI_SCAN_DONE);
    net_mgmt_add_event_callback(&wifi_filtered_scan_cb);

    int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params));
    if (ret) {
        net_mgmt_del_event_callback(&wifi_filtered_scan_cb);
        return ret;
    }

    ret = k_sem_take(&wifi_filtered_scan_done, K_FOREVER);
    net_mgmt_del_event_callback(&wifi_filtered_scan_cb);
    return ret;
}

int wifi_auto_connect(void)
{
    int ret;
    if (wifi_credentials_is_empty()) {
        return -ENOENT;
    }

    saved_ssids_count = 0;
    wifi_credentials_for_each_ssid(wifi_credentials_ssid_handler, NULL);

    if (saved_ssids_count == 0) {
        return -ENOENT;
    }

    ret = wifi_scan_filtered();
    if (ret) {
        return -ETIMEDOUT;
    }

    int last_best_rssi = 0;

    while (1) {
        int best_idx = -1;
        int current_best = INT_MIN;

        for (size_t i = 0; i < saved_ssids_count; i++) {
            const int rssi = saved_rssi[i];
            if (last_best_rssi > rssi && rssi > current_best) {
                current_best = rssi;
                best_idx = i;
            }
        }

        if (best_idx < 0) {
            break;
        }

        ret = wifi_connect((const char *)saved_ssids[best_idx]);
        if (ret == 0) {
            return 0;
        }

        last_best_rssi = current_best;

        // Without this next connect fails with status = -1 (UNKNOWN)
        k_msleep(500);
    }

    return -ENETUNREACH;
}