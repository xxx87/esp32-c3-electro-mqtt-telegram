#include "ota_handler.h"
#include "config.h"

#include <ArduinoOTA.h>
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFi.h>

static WebServer otaServer(80);

void otaSetup() {
  uint64_t chipId = ESP.getEfuseMac();
  uint32_t lastThreeBytes = (uint32_t)(chipId >> 24);
  snprintf(hostname, sizeof(hostname), "ESP32-C3-%06X", lastThreeBytes);

  WiFi.setHostname(hostname);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();

  ElegantOTA.begin(&otaServer);
  otaServer.begin();

  Serial.print("Hostname: ");
  Serial.println(hostname);
  Serial.print("ElegantOTA: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/update");
}

void otaLoop() {
  ArduinoOTA.handle();
  otaServer.handleClient();
  ElegantOTA.loop();
}
