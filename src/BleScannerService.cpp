#include "BleScannerService.h"
#include "Display.h"


static BLEScannerService *instance = nullptr;

void BLEScannerService::begin() {
    if(!BLEDevice::getInitialized()) {
        BLEDevice::init("");
    }
    scanner = BLEDevice::getScan();
    scanner->setAdvertisedDeviceCallbacks(this, false);
    scanner->setActiveScan(true);
    if (instance) return;
    instance = this;
    scanner->start(5, [](BLEScanResults r){instance->onScanComplete();}, false);
}

void BLEScannerService::onScanComplete() {
    scanner->clearResults();
    scanner->start(5, [](BLEScanResults r){instance->onScanComplete();}, false);
}

static uint32_t result_number = 0;
void BLEScannerService::onResult(BLEAdvertisedDevice advertisedDevice) {
    String name = advertisedDevice.getName();
    if(name.length() == 0) return;
    if (name.startsWith("BeeEye", 0) && advertisedDevice.haveManufacturerData()) {
        String mdata = advertisedDevice.getManufacturerData();
        if (mdata.length() >= 6) {
            struct {
                uint16_t magic;
                int16_t temp;
                int16_t hum;
            } __attribute__((packed)) data;

            memcpy(&data, mdata.c_str(), sizeof(data));
            if (data.magic == 0xBEEEu) {
                displayPrintf("B#%d %s", ++result_number, name.c_str());
                // displayPrintf("T=%.2f, H=%.2f\n", data.temp/256.0f, data.hum/256.0f);
                // displayPrintf("RSSI %ddBm", advertisedDevice.getRSSI());
            }
        }
    }
}