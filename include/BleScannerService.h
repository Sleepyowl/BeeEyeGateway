#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

class BLEScannerService: public BLEAdvertisedDeviceCallbacks {
public:
  BLEScannerService(const char* deviceName = "ESP32_Config") : _deviceName(deviceName) {}
  void begin();

private:
  void onResult(BLEAdvertisedDevice advertisedDevice) override;
  void onScanComplete();

  const char* _deviceName;
  BLEScan* scanner = nullptr;
};
