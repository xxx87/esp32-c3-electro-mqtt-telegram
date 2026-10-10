#include "wifi_setup.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiManager.h>

void wifiSetupAndLoadConfig() {
  preferences.begin("config", false);
  if (preferences.isKey("addr")) preferences.getString("addr", addr, 64);
  if (preferences.isKey("botToken")) preferences.getString("botToken", botToken, 64);
  if (preferences.isKey("chatId")) preferences.getString("chatId", chatId, 32);
  if (preferences.isKey("mqtt_srv")) preferences.getString("mqtt_srv", mqtt_server, 64);
  if (preferences.isKey("mqtt_u")) preferences.getString("mqtt_u", mqtt_user, 32);
  if (preferences.isKey("mqtt_p")) preferences.getString("mqtt_p", mqtt_pass, 32);
  if (preferences.isKey("mqtt_topic")) preferences.getString("mqtt_topic", mqtt_topic, 64);
  preferences.end();

  WiFiManager wm;

  WiFiManagerParameter custom_addr("addr", "Адреса об'єкта", addr, 64);
  WiFiManagerParameter custom_bot("bot", "Telegram Token", botToken, 64);
  WiFiManagerParameter custom_chat("chat", "Chat ID", chatId, 32);
  WiFiManagerParameter custom_mqtt_srv("mqtt", "MQTT Server IP", mqtt_server, 64);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT User", mqtt_user, 32);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 32);
  WiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", mqtt_topic, 64);

  wm.addParameter(&custom_addr);
  wm.addParameter(&custom_bot);
  wm.addParameter(&custom_chat);
  wm.addParameter(&custom_mqtt_srv);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);

  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("Power_Monitor_Setup")) {
    ESP.restart();
  }

  preferences.begin("config", false);
  preferences.putString("addr", custom_addr.getValue());
  preferences.putString("botToken", custom_bot.getValue());
  preferences.putString("chatId", custom_chat.getValue());
  preferences.putString("mqtt_srv", custom_mqtt_srv.getValue());
  preferences.putString("mqtt_u", custom_mqtt_user.getValue());
  preferences.putString("mqtt_p", custom_mqtt_pass.getValue());
  preferences.putString("mqtt_topic", custom_mqtt_topic.getValue());
  preferences.end();

  strcpy(addr, custom_addr.getValue());
  strcpy(botToken, custom_bot.getValue());
  strcpy(chatId, custom_chat.getValue());
  strcpy(mqtt_server, custom_mqtt_srv.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
}

void wifiFactoryResetIfHeld() {
  if (digitalRead(RESET_PIN) != LOW) {
    return;
  }

  unsigned long pressStart = millis();
  while (digitalRead(RESET_PIN) == LOW) {
    if (millis() - pressStart > 5000) {
      WiFiManager wm;
      wm.resetSettings();
      preferences.begin("config", false);
      preferences.clear();
      preferences.end();
      ESP.restart();
    }
    delay(10);
  }
}
