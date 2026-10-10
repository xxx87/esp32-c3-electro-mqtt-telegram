#pragma once

#include <Arduino.h>

void mqttSetup();
void mqttLoop();
bool mqttPublishPower(bool powerOn);
bool mqttIsConnected();
