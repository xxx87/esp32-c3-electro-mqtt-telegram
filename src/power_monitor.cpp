#include "power_monitor.h"
#include "config.h"
#include "mqtt_handler.h"
#include "telegram_handler.h"
#include "time_utils.h"
#include "wifi_setup.h"

#include <Arduino.h>

struct PowerEvent {
  uint8_t powerOn;
  uint64_t eventTime;
};

static bool lastStableState;
static bool pendingState;
static unsigned long stateChangedTime = 0;
static bool isWaitingForConfirm = false;

static PowerEvent eventQueue[MAX_PENDING_EVENTS];
static int eventQueueCount = 0;
static unsigned long lastPendingRetry = 0;

static void loadEventQueue() {
  preferences.begin("pwr-mon", true);
  eventQueueCount = preferences.getInt("q_cnt", 0);
  if (eventQueueCount < 0) {
    eventQueueCount = 0;
  }
  if (eventQueueCount > MAX_PENDING_EVENTS) {
    eventQueueCount = MAX_PENDING_EVENTS;
  }

  for (int i = 0; i < eventQueueCount; i++) {
    char onKey[8];
    char tKey[8];
    snprintf(onKey, sizeof(onKey), "q%d_on", i);
    snprintf(tKey, sizeof(tKey), "q%d_t", i);
    eventQueue[i].powerOn = preferences.getUChar(onKey, 0);
    eventQueue[i].eventTime = preferences.getULong64(tKey, 0);
  }
  preferences.end();
}

static void saveEventQueue() {
  preferences.begin("pwr-mon", false);
  preferences.putInt("q_cnt", eventQueueCount);
  for (int i = 0; i < eventQueueCount; i++) {
    char onKey[8];
    char tKey[8];
    snprintf(onKey, sizeof(onKey), "q%d_on", i);
    snprintf(tKey, sizeof(tKey), "q%d_t", i);
    preferences.putUChar(onKey, eventQueue[i].powerOn);
    preferences.putULong64(tKey, eventQueue[i].eventTime);
  }
  preferences.end();
}

static void persistKnownState(bool powerOn, time_t sinceTime) {
  preferences.begin("pwr-mon", false);
  preferences.putUChar("known_on", powerOn ? 1 : 0);
  preferences.putULong64("since_t", (uint64_t)sinceTime);
  preferences.end();
}

static void enqueuePowerEvent(bool powerOn, time_t eventTime) {
  if (eventQueueCount >= MAX_PENDING_EVENTS) {
    memmove(&eventQueue[0], &eventQueue[1], sizeof(PowerEvent) * (MAX_PENDING_EVENTS - 1));
    eventQueueCount = MAX_PENDING_EVENTS - 1;
  }

  eventQueue[eventQueueCount].powerOn = powerOn ? 1 : 0;
  eventQueue[eventQueueCount].eventTime = (uint64_t)eventTime;
  eventQueueCount++;
  saveEventQueue();
  persistKnownState(powerOn, eventTime);

  Serial.print("Queued power event: ");
  Serial.print(powerOn ? "ON" : "OFF");
  Serial.print(" at ");
  Serial.println(getFormattedTime(eventTime));
}

static bool deliverPowerEvent(const PowerEvent &event) {
  const bool powerOn = event.powerOn != 0;
  const time_t eventTime = (time_t)event.eventTime;

  preferences.begin("pwr-mon", true);
  time_t lastTime = (time_t)preferences.getULong64("last_t", 0);
  preferences.end();

  String message = "🏠 " + String(addr) + (powerOn ? "\n\n🟢 *Живлення відновлено*" : "\n\n🔴 *Живлення відключено*");
  message += "\n🕙 В: " + getFormattedTime(eventTime);

  if (lastTime > 0 && eventTime > lastTime) {
    time_t diff = eventTime - lastTime;
    String statusWord = powerOn ? "⌛️ Без світла: " : "⌛️Зі світлом: ";
    message += "\n\n" + statusWord + getDuration(diff);
  }

  if (!telegramSendMessage(message, "Markdown")) {
    Serial.println("Failed to send Telegram message");
    return false;
  }

  const bool mqttRequired = strlen(mqtt_server) > 0 && strlen(mqtt_topic) > 0;
  if (mqttRequired && !mqttPublishPower(powerOn)) {
    Serial.println("Failed to publish MQTT power state");
    return false;
  }

  preferences.begin("pwr-mon", false);
  preferences.putULong64("last_t", (uint64_t)eventTime);
  preferences.end();
  return true;
}

static void flushPendingEvents() {
  if (eventQueueCount == 0) {
    return;
  }

  if (millis() - lastPendingRetry < PENDING_RETRY_MS && lastPendingRetry != 0) {
    return;
  }
  lastPendingRetry = millis();

  if (!wifiHasInternet()) {
    Serial.println("Pending power events waiting for internet");
    return;
  }

  while (eventQueueCount > 0) {
    if (!deliverPowerEvent(eventQueue[0])) {
      Serial.println("Keeping power events queued until delivery succeeds");
      break;
    }

    memmove(&eventQueue[0], &eventQueue[1], sizeof(PowerEvent) * (eventQueueCount - 1));
    eventQueueCount--;
    saveEventQueue();
    Serial.println("Delivered queued power event");
  }
}

static void recoverStateAfterPowerLoss() {
  time_t now;
  time(&now);
  if (now < 1000000) {
    Serial.println("Skip boot recovery: time not synced");
    return;
  }

  const bool current = powerMonitorIsOn();

  preferences.begin("pwr-mon", true);
  const bool hasKnown = preferences.isKey("known_on");
  const bool knownOn = preferences.getUChar("known_on", current ? 1 : 0) != 0;
  preferences.end();

  if (!hasKnown) {
    persistKnownState(current, now);
    Serial.println("Initialized known power state");
    return;
  }

  if (knownOn == current) {
    return;
  }

  Serial.println("Power state changed while device was offline, queueing event");
  enqueuePowerEvent(current, now);
  lastPendingRetry = 0;
}

bool powerMonitorIsOn() {
  return digitalRead(SENSOR_PIN) == LOW;
}

time_t powerMonitorStateSince() {
  preferences.begin("pwr-mon", true);
  time_t since = (time_t)preferences.getULong64("since_t", 0);
  preferences.end();
  return since;
}

void powerMonitorSetup() {
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  lastStableState = powerMonitorIsOn();
  pendingState = lastStableState;
  loadEventQueue();
  if (eventQueueCount > 0) {
    Serial.print("Loaded pending power events: ");
    Serial.println(eventQueueCount);
  }
  recoverStateAfterPowerLoss();
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

        time_t now;
        time(&now);
        if (now < 1000000) {
          Serial.println("Skip queue: time not synced");
        } else {
          enqueuePowerEvent(lastStableState, now);
          lastPendingRetry = 0;
        }
      }
    }
  } else {
    isWaitingForConfirm = false;
  }

  flushPendingEvents();
}
