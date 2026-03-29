#pragma once
#include <stdint.h>

bool initWifi();
bool connectWifi(uint32_t timeout_ms = 10000);
void scanWifiNetworks();
void autoAdjustWiFiPower();