/*************************************
**  For ESP32 C6
**
**  Use serial console to configure 
**    - WiFi
**    - Time zone
*************************************/
#include "Wiring.h"
#include "Display.h"
#include "LedStatus.h"
#include "SerialConsole.h"
#include "AppConfig.h"
#include "WiFiUtil.h"
#include "messages.h"
#include "BleConfigService.h"
#include "BleScannerService.h"
#include "InfluxSender.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <esp_system.h>
#include <RadioLib.h>
#include <esp32-hal-timer.h>


#include <esp_log.h>

/*************************************
**  WIRING AND CONSTANTS
*************************************/
constexpr uint32_t TimeSyncPeriod = 3600 * 24 * 7; // Sync time every week


/*************************************
**  CONSOLE COMMANDS 
**  impl in CliHandlers.cpp
*************************************/
void cmdStatus(const char* arg);
void cmdConnect(const char* arg);
void cmdSave(const char* arg);
void cmdDump(const char* arg);
void cmdRestart(const char* arg);
void cmdSetSsid(const char* arg);
void cmdSetPass(const char* arg);
void cmdSetTz(const char* arg);
void cmdScanWifi(const char* arg);
void cmdSetInfluxUrl(const char* arg);
void cmdSetInfluxUser(const char* arg);
void cmdSetInfluxPass(const char* arg);

CommandEntry consoleCommands[] = {
  {"status",    cmdStatus,             "status              - display unit status"},
  {"connect",   cmdConnect,            "connect             - connect to configured WiFi"},
  {"save",      cmdSave,               "save                - save current config to flash"},
  {"dump",      cmdDump,               "dump                - dump current config"},
  {"restart",   cmdRestart,            "restart             - reboot device"},
  {"wifissid",  cmdSetSsid,            "wifissid <ssid>     - set WiFi SSID"},
  {"wifipass",  cmdSetPass,            "wifipass <pass>     - set WiFi password"},
  {"wifiscan",  cmdScanWifi,           "wifiscan            - scan available WiFi networks"},
  {"timezone",  cmdSetTz,              "timezone +-HH:MM    - sets timezone"},
  {"influxurl", cmdSetInfluxUrl,       "influxurl <url>     - set InfluxDB server URL (max 256 chars)"},
  {"influxuser",cmdSetInfluxUser,      "influxuser <user>   - set InfluxDB username"},
  {"influxpass",cmdSetInfluxPass,      "influxpass <pass>   - set InfluxDB password"}
};

/*************************************
**  RTC MODULE
*************************************/
RTC_DS3231 ds3231;

bool isDST_EU(const DateTime& date) {
  int y = date.year();
  int m = date.month();
  int d = date.day();
  int wday = date.dayOfTheWeek();

  if (m < 3 || m > 10) {
    return false;
  }

  if (m > 3 && m < 10) {
    return true;
  }

  // Weekday of 31st of this month 
  // (both March and October has 31 days)
  DateTime lastDay(y, m, 31, 0, 0, 0);
  int wdayLast = lastDay.dayOfTheWeek();
  int lastSunday = 31 - wdayLast;

  if (m == 3) {
    return d >= lastSunday;  // from last Sunday in March
  }
  else {
    return d < lastSunday;   // until last Sunday in October
  }
}

DateTime getLocalTime(const DateTime& utcTime) {
  const uint32_t timestamp = utcTime.unixtime(); 
  const int16_t offset = (appConfig.timeZoneOffset + isDST_EU(utcTime) * 60) * 60;
  return DateTime(timestamp + offset); 
}

/*************************************
**  NTP UTILITY
*************************************/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
uint32_t lastNtpSync;

bool syncTime(int &delta) {    
  if(!WiFi.isConnected()) return false;
  WiFi.setSleep(false);
  autoAdjustWiFiPower();
  if(timeClient.update()) {
    const DateTime rtcTime = ds3231.now();
    const DateTime ntpTime(timeClient.getEpochTime());    
    ds3231.adjust(ntpTime);
    const auto ntpTimestamp = ntpTime.unixtime();
    const auto rtcTimestamp = rtcTime.unixtime();
    delta = ntpTimestamp - rtcTimestamp; // beware arithmetic
    lastNtpSync = ntpTimestamp;
    WiFi.setSleep(true);
    return true;
  }  
  WiFi.setSleep(true);
  return false;
}

bool syncTime() {
  int delta;
  return syncTime(delta);
}

/*************************************
**  setup() and loop()
*************************************/
BLEConfigService bleConfig("LoRa Base Config");
BLEScannerService bleScanner("");
SerialConsole cli = SERIAL_CONSOLE(consoleCommands);
SX1262 lora = new Module(Wiring::SX1262::CS, Wiring::SX1262::DIO1, Wiring::SX1262::RST, Wiring::SX1262::BUSY);

volatile bool rxFlag = false;
void IRAM_ATTR onLoraRx() { 
  rxFlag = true; 
}

bool setupRadio() {
  const int state = lora.begin(Wiring::SX1262::FREQ, 125.0, 9U, 7U, 0x12, 10, 8U);
  if (state != RADIOLIB_ERR_NONE) {    
    Serial.printf("LoRa init failed, status=%d\n", state);
    return false;
  }  
  lora.setCRC(true);  
  lora.setDio2AsRfSwitch(true);
  lora.setTCXO(3.0, 5000);
  lora.setRxBoostedGainMode(true, true);
  lora.setRegulatorLDO();
  return true;
}

hw_timer_t *timer = NULL;
volatile uint32_t uptime_seconds = 0;

void IRAM_ATTR onTimer() {
  __atomic_add_fetch(&uptime_seconds, 1, __ATOMIC_RELAXED);
}

void setup() {  
  statusLedBegin();
  setPixelSignal(PixelSignal::Initializing);

  Serial.begin(9600);
  delay(1000);
  Serial.printf("Reset reason: %d\n", esp_reset_reason());

  // Init display
  Wire.begin(Wiring::SDA, Wiring::SCL);
  if (!displayBegin()) {
    Serial.println("SSD1306 init failed");    
    setPixelSignal(PixelSignal::Error);
    while (1);
  }

  displayPrint("Sensor Station v1.0");

  // Load config from NVM
  if(!appConfig.load()) {
    displayPrint("[FAIL] load config");
  } else {
    displayPrint("[OK] load config");
  }

  // Start BLE interface
  // bleConfig.begin(&appConfig);
  // bleScanner.begin();
  // displayPrint("[OK] BLE");

  // Start Radio
  displayPrint("Initializing LoRa... ");    
  SPI.begin(Wiring::SX1262::SCK, Wiring::SX1262::MISO, Wiring::SX1262::MOSI, Wiring::SX1262::CS);
  if (!setupRadio()) {    
    displayPrint("[FAILED] LORA");
  } else {
    displayPrint("[OK] LORA");
    lora.setDio1Action(onLoraRx); 
    pinMode(Wiring::SX1262::RFSW, OUTPUT);
    digitalWrite(Wiring::SX1262::RFSW, HIGH);
    lora.startReceive();
  }
  

  // // Start RTC module  
  // if (!ds3231.begin()) {
  //   displayFatalError("[FAIL] RTC");
  //   while (1) delay(10);
  // }
  // const auto now = ds3231.now();
  // displayPrintf("RTC: %d:%d:%d UTC", now.hour(), now.minute(), now.second());
  // displayPrint(ds3231.lostPower() ? "[WARN] RTC PWR" : "[OK] RTC");

  // Start timer (instead of RTC)
  timer = timerBegin(1000);         
  if (!timer) {
    displayPrint("[FATAL] timerBegin failed");
    while (true) {}
  }
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);   

  // WIFI
  initWifi();

  // Make sure WiFi and BT are off
  //WiFi.mode(WIFI_OFF); 

  // // NTP
  // if(WiFi.isConnected()) {  
  //   int delta = 0;
  //   if(syncTime(delta)) {
  //     displayPrint("[OK] NTP sync");
  //     displayPrintf("delta=%d", delta);
  //   } else {
  //     displayPrint("[FAIL] NTP");
  //   }    
  // } else {
  //   displayPrint("[SKIP] NTP");
  // }

  // Start UART interface  
  cli.begin();
  displayPrint("[OK] serial cli");

  setPixelSignal(PixelSignal::Ok);  
  displayPrint("Station Ready");
  // displayPrintf("Uptime = %d", uptime_seconds);
  delay(1000);
  setPixelSignal(PixelSignal::None);    
}

void printLinkStats() {
  // const auto now = getLocalTime(ds3231.now());
  // displayPrintf("%02d:%02d:%02d Received", now.hour(), now.minute(), now.second());
  //displayPrintf("+%d Received", uptime_seconds);
  // displayPrintf("+%02d:%02d:%02d Received", uptime_seconds / 3600, (uptime_seconds % 3600) / 60, uptime_seconds % 60);
  // displayPrintf("%.1fdBm (%.1fdB)", lora.getRSSI(true), lora.getSNR()); 

  displayPrintf("+%02d:%02d:%02d %.1fdBm", uptime_seconds / 3600, (uptime_seconds % 3600) / 60, uptime_seconds % 60, lora.getRSSI());

}
constexpr size_t RX_BUFFER = 256;
uint8_t rxBuffer[RX_BUFFER + 1] = {};
void loop() {
  if (!rxFlag) return;
  rxFlag = false;

  printLinkStats();

  String str;
  const size_t packetSize = lora.getPacketLength();
  if(packetSize > RX_BUFFER) {
    displayPrintf("RX fail: packet is too long");
    uint8_t ignore;
    lora.readData(&ignore, 1);
    return;
  }  
  
  int state = lora.readData(rxBuffer, packetSize);
  rxBuffer[packetSize] = 0;

  if (state == RADIOLIB_ERR_NONE) {        
    const auto header = reinterpret_cast<const MessageHeader*>(rxBuffer);
    if(*reinterpret_cast<const uint32_t*>(header->magic) != MAGIC_BEYE) {
      displayPrint("Invalid magic");
    } else {    
      if(header->type == MessageType::Text) {
        displayPrintf("TXT: %s", reinterpret_cast<const char*>(rxBuffer + sizeof(MessageHeader)));      
      } else if (header->type == MessageType::Measure){
        const auto measure_count = (packetSize - sizeof(MessageHeader)) / sizeof(Measure);
        for(int i = 0; i < measure_count; ++i) {
          const auto measure = reinterpret_cast<const Measure*>(rxBuffer + sizeof(MessageHeader) + sizeof(Measure)*i);
          switch (measure->type)
          {
          case MeasureType::Battery:
            displayPrintf("Vbat: %dmV", measure->_data.mV);
            break;
          case MeasureType::Temperature:
            displayPrintf("1w|%02X%02X: %.2fC", 
              measure->sensorAddress[6], 
              measure->sensorAddress[7], 
              measure->_data.tempC);
            break;
          case MeasureType::TemperatureAndHumidity:
            displayPrintf("bt|%02X%02X: %.1fC, %.1f%%", 
              measure->sensorAddress[6], 
              measure->sensorAddress[7], 
              measure->_data.tempC, 
              measure->hum);
            break;
          default:
            displayPrintf("?: 0x%08X", measure->_data.raw);
            break;
          }
          processMeasure(*measure);
        }
      }
    }    
  } else {
    displayPrintf("RX fail %d", state);
  }

  lora.startReceive();            // re-arm for the next one
}