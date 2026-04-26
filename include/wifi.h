#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize Wi-Fi subsystem.
 *
 * Sets up internal state and registers required callbacks.
 * Must be called before using any other function in this interface.
 *
 * @retval 0        Success
 * @retval -errno   Initialization failed
 */
int wifi_init(void);

/**
 * @brief Check Wi-Fi connection status.
 *
 * @return true if connected to an access point, false otherwise.
 */
bool wifi_is_connected(void);

/**
 * @brief Connect to a specific Wi-Fi network.
 *
 * Attempts to connect to the given SSID using previously stored
 * credentials (added via the Wi-Fi credentials subsystem).
 *
 * @param[in] ssid  Null-terminated SSID string
 *
 * @retval 0        Connection attempt started successfully
 * @retval -ENOENT  No credentials found for given SSID
 * @retval -EINVAL  Invalid parameter
 * @retval -errno   Connection request failed
 */
int wifi_connect(const char *ssid);

/**
 * @brief Connect to any known Wi-Fi network.
 *
 * Iterates through stored credentials and attempts to connect
 * to available networks until a connection is established or
 * all options are exhausted.
 *
 * @retval 0        Connection attempt started successfully
 * @retval -ENOENT  No stored credentials available
 * @retval -errno   Auto-connect process failed
 */
int wifi_auto_connect(void);