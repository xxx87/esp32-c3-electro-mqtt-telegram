#include "power_monitor.h"
#include "config.h"
#include "mqtt_handler.h"
#include "telegram_handler.h"
#include "time_utils.h"

#include <Arduino.h>

static bool lastStableState;
static bool pendingState;
static unsigned long stateChangedTime = 0;
static bool isWaitingForConfirm = false;

bool powerMonitorIsOn() {
  return digitalRead(SENSOR_PIN) == LOW;
}

void sendStatusUpdate(bool powerOn) {
  time_t now;
  time(&now);
  if (now < 1000000) {
    return;
  }

  preferences.begin("pwr-mon", false);
  time_t lastTime = (time_t)preferences.getULong64("last_t", 0);

  String message = "🏠 " + String(addr) + (powerOn ? "\n\n🟢 *Живлення відновлено*" : "\n\n🔴 *Живлення відключено*");
  message += "\n🕙 В: " + getFormattedTime(now);

  if (lastTime > 0) {
    time_t diff = now - lastTime;
    String statusWord = powerOn ? "⌛️ Без світла: " : "⌛️Зі світлом: ";
    message += "\n\n" + statusWord + getDuration(diff);
  }

  if (!telegramSendMessage(message, "Markdown")) {
    Serial.println("Failed to send Telegram message");
  }

  mqttPublishPower(powerOn);

  preferences.putULong64("last_t", (uint64_t)now);
  preferences.end();
}

void powerMonitorSetup() {
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  lastStableState = powerMonitorIsOn();
  pendingState = lastStableState;
}

void powerMonitorLoop() {
  bool rawState = powerMonitorIsOn();

  if (rawState != lastStableState) {
    if (!isWaitingForConfirm || rawState != pendingState) {
      isWaitingForConfirm = true;
      pendingState = rawState;
      stateChangedTime = millis();
    }

    if (isWaitingForConfirm && (millis() - stateChangedTime >= CONFIRM_DELAY)) {
      if (pendingState != lastStableState) {
        lastStableState = pendingState;
        isWaitingForConfirm = false;
        sendStatusUpdate(lastStableState);
      }
    }
  } else {
    isWaitingForConfirm = false;
  }
}
