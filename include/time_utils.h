#pragma once

#include <Arduino.h>
#include <time.h>

String getDuration(time_t diff);
String getFormattedTime(time_t t);
void setupNtpTime();
