#include <Arduino.h>

#include "config.h"
#include "wifi_setup.h"
#include "time_utils.h"
#include "mqtt_handler.h"
#include "telegram_handler.h"
#include "ota_handler.h"
#include "power_monitor.h"

void setup() {
  Serial.begin(115200);
  pinMode(RESET_PIN, INPUT_PULLUP);

  wifiSetupAndLoadConfig();
  mqttSetup();
  setupNtpTime();
  telegramSetup();
  otaSetup();
  powerMonitorSetup();

  Serial.println("System ready");
}

void loop() {
  otaLoop();
  telegramLoop();
  wifiFactoryResetIfHeld();
  mqttLoop();
  powerMonitorLoop();
  delay(100);
}
