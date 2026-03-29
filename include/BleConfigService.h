#pragma once
#include "AppConfig.h"

#include <BLEDevice.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLEServer.h>
#include <BLEUtils.h>

class BLEConfigService : public BLECharacteristicCallbacks, BLEServerCallbacks {
public:
  BLEConfigService(const char* deviceName = "ESP32_Config") : _deviceName(deviceName) {}
  void begin(AppConfig* cfg);

private:
  const char* _deviceName;
  BLEServer* _server = nullptr;
  BLEService* _service = nullptr;
  BLECharacteristic* _configChar = nullptr;
  AppConfig* _cfg = nullptr;

  void onWrite(BLECharacteristic* c) override;
  void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) override;
  void onDisconnect(BLEServer *pServer, ble_gap_conn_desc *desc) override;
};
