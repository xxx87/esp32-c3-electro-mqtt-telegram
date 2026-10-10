#include "config.h"

char hostname[32];         // Device mDNS/OTA hostname: ESP32-C3-FC98BA
char addr[64] = "";        // Object address shown in Telegram: vul. Stadionna, 6A
char botToken[64] = "";    // Telegram bot token from @BotFather: 123456:ABC-DEF...
char chatId[32] = "";      // Telegram chat/group ID for alerts: -1001234567890
char mqtt_server[64] = ""; // MQTT broker host or IP: mqtt.example.com
char mqtt_user[32] = "";   // MQTT username: user1
char mqtt_pass[32] = "";   // MQTT password: secret
char mqtt_topic[64] = "";  // MQTT topic for ON/OFF status: home/power/stadium

Preferences preferences; // NVS storage for WiFiManager settings
