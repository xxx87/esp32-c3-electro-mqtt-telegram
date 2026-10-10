#include "ota_handler.h"
#include "config.h"

#include <WiFi.h>
#include <ArduinoOTA.h>

void otaSetup() {
  uint64_t chipId = ESP.getEfuseMac();
  uint32_t lastThreeBytes = (uint32_t)(chipId >> 24);
  snprintf(hostname, sizeof(hostname), "ESP32-C3-%06X", lastThreeBytes);

  WiFi.setHostname(hostname);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();

  Serial.print("Hostname: ");
  Serial.println(hostname);
}

void otaLoop() {
  ArduinoOTA.handle();
}
