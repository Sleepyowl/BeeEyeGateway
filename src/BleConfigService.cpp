#include "BleConfigService.h"



#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
void BLEConfigService::begin(AppConfig* cfg) {
  _cfg = cfg;
  BLEDevice::init(_deviceName);

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setPassKey(true, 123123);
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setAuthenticationMode(true, true, true);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  _server = BLEDevice::createServer();
  _server->advertiseOnDisconnect(true);
  _server->setCallbacks(this);
  _service = _server->createService(SERVICE_UUID);
  _configChar = _service->createCharacteristic(CHARACTERISTIC_UUID, 
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_READ_AUTHEN | BLECharacteristic::PROPERTY_WRITE_AUTHEN);

  _configChar->setCallbacks(this);
  _configChar->setValue((uint8_t*)_cfg, sizeof(AppConfig));


  _service->start();

  _server->getAdvertising()->start();
}

void BLEConfigService::onWrite(BLECharacteristic* c) {
  String val = c->getValue();
  if (val.length() != sizeof(AppConfig)) return; // ignore invalid writes
  memcpy(_cfg, val.c_str(), sizeof(AppConfig));
  _cfg->updateCrc();
  _cfg->save();
}

void BLEConfigService::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
  Serial.println("BLE Client Connected");
  Serial.printf("enc=%d, auth=%d, bonded=%d\n",
    desc->sec_state.encrypted,
    desc->sec_state.authenticated,
    desc->sec_state.bonded);
}

void BLEConfigService::onDisconnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
  Serial.println("BLE Client Disconnected");
  // _server->getAdvertising()->start();
}