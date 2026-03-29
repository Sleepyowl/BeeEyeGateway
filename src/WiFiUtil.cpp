#include "AppConfig.h"
#include "Display.h"
#include "WiFiUtil.h"

#include <WiFi.h>

SemaphoreHandle_t wifiLock = nullptr;

bool initWifi() {
  wifiLock = xSemaphoreCreateMutex();
  if (!wifiLock) {
    displayFatalError("Couldn't create mutex");
    while (1) delay(10);
  }  
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  if(strlen(appConfig.ssid) > 0) {
    displayPrintf("Connecting to %s...", appConfig.ssid);
    if(connectWifi()) {      
      displayPrintf("Connected to %s (%d dBm)", appConfig.ssid, WiFi.RSSI());
    }
  } else {    
    displayPrint("[FATAL] No WiFi settings found");
    displayPrint("Use serial console to configure");
  }
  return true;
}

void scanWifiNetworks() {
  if (!wifiLock) { Serial.println("WiFi mutex not initialized"); return; }
  if (xSemaphoreTake(wifiLock, portMAX_DELAY) != pdTRUE) {
    Serial.println("Failed to acquire WiFi mutex");
    return;
  }

  Serial.println("WiFi scan…");
  // Assumes WiFi.mode(WIFI_STA) already set in setup()
  const int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);
  if (n < 0) {
    Serial.println("WiFi scan failed");
    xSemaphoreGive(wifiLock);
    return;
  }

  for (int i = 0; i < n; ++i) {
    Serial.printf("%2d) %s  %ld dBm\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
  WiFi.scanDelete();

  xSemaphoreGive(wifiLock);
}

bool connectWifi(uint32_t timeout_ms) {
  if (!wifiLock) { 
    Serial.println("WiFi mutex not initialized"); 
    return false; 
  }
  
  if (strlen(appConfig.ssid) == 0) { 
    Serial.println("SSID not set"); 
    return false; 
  }

  const auto passLen = strlen(appConfig.pass);
  if (passLen < 8 || passLen > 63) {
    Serial.println("Password length invalid (8–63)");
    return false;
  }

  if (xSemaphoreTake(wifiLock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
    Serial.println("WiFi mutex timeout");
    return false;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == appConfig.ssid) {
    xSemaphoreGive(wifiLock);
    return true; // already connected to desired SSID
  }

  WiFi.begin(appConfig.ssid, appConfig.pass);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  bool ok = (WiFi.status() == WL_CONNECTED);
  if (!ok) { 
    Serial.println("WiFi connect timeout"); 
  }
  xSemaphoreGive(wifiLock);
  return ok;
}

void autoAdjustWiFiPower() {
  if (WiFi.status() != WL_CONNECTED) return;
  int rssi = WiFi.RSSI();

  wifi_power_t pwr =
      (rssi > -50) ? WIFI_POWER_2dBm :
      (rssi > -65) ? WIFI_POWER_8_5dBm : WIFI_POWER_15dBm;   // only if you truly need it

  WiFi.setTxPower(pwr);
}
