#include "Display.h"
#include "AppConfig.h"
#include "messages.h"

#include <Arduino.h>
#include <stdio.h>
#include <WiFi.h>
#include <HTTPClient.h>

char body[1024];
void processMeasure(const struct Measure& measure) {
  char saddr[17];
  snprintf(saddr, sizeof(saddr), "%02x%02x%02x%02x%02x%02x%02x%02x", 
    measure.sensorAddress[0],
    measure.sensorAddress[1],
    measure.sensorAddress[2],
    measure.sensorAddress[3],
    measure.sensorAddress[4],
    measure.sensorAddress[5],
    measure.sensorAddress[6],
    measure.sensorAddress[7]
  );


  if (measure.type == MeasureType::Temperature) {
    snprintf(body, sizeof(body), "temperature,sensor=%s value=%f", saddr, measure._data.tempC);
  } else if (measure.type == MeasureType::TemperatureAndHumidity) {
    snprintf(body, sizeof(body), "temperature,sensor=%s value=%f\nhumidity,sensor=%s value=%f", saddr, measure._data.tempC, saddr, measure.hum);
  } else {
    return;
  }

  Serial.printf("Body: %s\n", body);

  if(WiFi.status()== WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
  
    http.begin(client, appConfig.influxUrl);
    //http.setAuthorization("sensors", "BananaGoat17+");
    http.setAuthorization(appConfig.influxUser, appConfig.influxPass);
    int httpResponseCode = http.POST(body);
    Serial.print("Posting metrics response code: ");
    Serial.println(httpResponseCode);
    http.end();
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
    } else {
      displayPrintf("Posting metric failed, httpcode = %d", httpResponseCode);
    }
  } else {
    displayPrintf("Posting metric failed: no Wifi");
  }
}