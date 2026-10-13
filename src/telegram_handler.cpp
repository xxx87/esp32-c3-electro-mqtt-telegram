#include "telegram_handler.h"
#include "config.h"
#include "power_monitor.h"
#include "time_utils.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

static WiFiClientSecure secClient;
static UniversalTelegramBot bot("", secClient);
static unsigned long lastBotCheck = 0;
static int botUpdateFailures = 0;

static void handleParamCommands(const String &text) {
  if (!text.startsWith("/set_mqtt")) {
    return;
  }

  int firstSpace = text.indexOf(' ');
  if (firstSpace == -1) {
    return;
  }

  String params = text.substring(firstSpace + 1);
  params.trim();

  int firstSemi = params.indexOf(';');
  int secondSemi = params.indexOf(';', firstSemi + 1);
  int thirdSemi = params.indexOf(';', secondSemi + 1);

  if (firstSemi > 0 && secondSemi > 0 && thirdSemi > 0) {
    String newSrv = params.substring(0, firstSemi);
    String newUser = params.substring(firstSemi + 1, secondSemi);
    String newPass = params.substring(secondSemi + 1, thirdSemi);
    String newTopic = params.substring(thirdSemi + 1);

    preferences.begin("config", false);
    preferences.putString("mqtt_srv", newSrv);
    preferences.putString("mqtt_u", newUser);
    preferences.putString("mqtt_p", newPass);
    preferences.putString("mqtt_topic", newTopic);
    preferences.end();

    bot.sendMessage(chatId, "✅ Параметри MQTT оновлено!\nСервер: " + newSrv + "\nПерезавантажуюсь...", "");
    delay(2000);
    ESP.restart();
  } else {
    bot.sendMessage(chatId, "❌ Помилка формату!\nВикористовуй: /set_mqtt сервер;юзер;пароль;топік", "");
  }
}

static void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String target_id = String(chatId);
    target_id.trim();

    if (!chat_id.equals(target_id)) {
      Serial.println("Chat ID mismatch, ignore");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.print("Command: ");
    Serial.println(text);

    if (text.startsWith("/status")) {
      time_t now;
      time(&now);

      bool currentPower = powerMonitorIsOn();
      time_t stateSince = powerMonitorStateSince();
      String stat = currentPower ? "✅ Світло є" : "❌ Світла немає";

      String reply = "📍 " + String(addr) + "\n";
      reply += "Статус: " + stat + "\n";

      if (stateSince > 0 && now >= stateSince) {
        reply += (currentPower ? "Зі світлом вже: " : "Без світла вже: ");
        reply += getDuration(now - stateSince) + "\n";
      }

      reply += "📡 Сигнал Wi-Fi: " + String(WiFi.RSSI()) + " dBm";
      if (!bot.sendMessage(chatId, reply, "Markdown")) {
        Serial.println("Failed to send status reply");
      }
    }

    if (text.startsWith("/reboot")) {
      bot.sendMessage(chatId, String(addr) + "\n\n🔄 Перезавантажуюсь...", "");
      delay(500);
      ESP.restart();
    }

    handleParamCommands(text);
  }
}

void telegramSetup() {
  secClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  bot.updateToken(botToken);
  bot.waitForResponse = 5000;
  secClient.setTimeout(10000);

  delay(1000);

  int result = bot.getUpdates(bot.last_message_received + 1);
  if (result >= 0) {
    Serial.println("Telegram bot initialized");
  } else {
    Serial.println("Warning: Failed to initialize Telegram bot");
  }
}

bool telegramSendMessage(const String &message, const String &parseMode) {
  if (strlen(botToken) == 0 || strlen(chatId) == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }
  return bot.sendMessage(chatId, message, parseMode);
}

void telegramLoop() {
  if (millis() <= lastBotCheck + BOT_INTERVAL_MS) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && strlen(botToken) > 0 && strlen(chatId) > 0) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages > 0) {
      Serial.print("Received ");
      Serial.print(numNewMessages);
      Serial.println(" new messages");
      handleNewMessages(numNewMessages);
      botUpdateFailures = 0;

      int remaining = bot.getUpdates(bot.last_message_received + 1);
      int loopCount = 0;
      while (remaining > 0 && loopCount < 10) {
        handleNewMessages(remaining);
        remaining = bot.getUpdates(bot.last_message_received + 1);
        loopCount++;
      }
    } else if (numNewMessages < 0) {
      botUpdateFailures++;
      Serial.print("Bot update failed, count: ");
      Serial.println(botUpdateFailures);

      if (botUpdateFailures >= MAX_BOT_FAILURES) {
        Serial.println("Reinitializing bot connection...");
        secClient.stop();
        delay(500);
        bot.updateToken(botToken);
        botUpdateFailures = 0;
      }
    } else {
      botUpdateFailures = 0;
    }
  } else {
    Serial.println("WiFi not connected, skipping bot check");
  }

  lastBotCheck = millis();
}
