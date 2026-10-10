#pragma once

void powerMonitorSetup();
void powerMonitorLoop();
bool powerMonitorIsOn();
void sendStatusUpdate(bool powerOn);
