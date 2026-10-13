#pragma once

#include <Preferences.h>

extern char hostname[32];
extern char addr[64];
extern char botToken[64];
extern char chatId[32];
extern char mqtt_server[64];
extern char mqtt_user[32];
extern char mqtt_pass[32];
extern char mqtt_topic[64];

extern Preferences preferences;

constexpr int RESET_PIN = 9;
constexpr int SENSOR_PIN = 2;
constexpr unsigned long CONFIRM_DELAY = 5000;
constexpr unsigned long BOT_INTERVAL_MS = 1000;
constexpr int MAX_BOT_FAILURES = 5;
constexpr int MQTT_PORT = 8883;
constexpr unsigned long MQTT_PUBLISH_INTERVAL_MS = 20000;
constexpr int MAX_PENDING_EVENTS = 4;
constexpr unsigned long PENDING_RETRY_MS = 15000;
constexpr unsigned long INTERNET_CHECK_CACHE_MS = 10000;
