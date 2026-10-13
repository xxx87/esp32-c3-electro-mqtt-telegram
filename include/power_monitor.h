#pragma once

#include <time.h>

void powerMonitorSetup();
void powerMonitorLoop();
bool powerMonitorIsOn();
time_t powerMonitorStateSince();
