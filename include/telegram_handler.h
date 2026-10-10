#pragma once

#include <Arduino.h>

void telegramSetup();
void telegramLoop();
bool telegramSendMessage(const String &message, const String &parseMode = "");
