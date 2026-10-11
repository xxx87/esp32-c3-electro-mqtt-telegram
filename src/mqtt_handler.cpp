#include "mqtt_handler.h"
#include "config.h"
#include "power_monitor.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure mqttSecClient;
static PubSubClient mqttClient(mqttSecClient);
static unsigned long lastMqttPublish = 0;

void mqttSetup() {
  mqttSecClient.setInsecure();
  mqttClient.setServer(mqtt_server, MQTT_PORT);
}

void mqttLoop() {
  if (!mqttClient.connected() && strlen(mqtt_server) > 0) {
    mqttClient.connect(hostname, mqtt_user, mqtt_pass);
  }
  mqttClient.loop();

  if (mqttClient.connected() && millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL_MS) {
    mqttPublishPower(powerMonitorIsOn());
  }
}

bool mqttIsConnected() {
  return mqttClient.connected();
}

bool mqttPublishPower(bool powerOn) {
  if (!mqttClient.connected() || strlen(mqtt_topic) == 0) {
    return false;
  }
  bool ok = mqttClient.publish(mqtt_topic, powerOn ? "ON" : "OFF", true);
  if (ok) {
    lastMqttPublish = millis();
  }
  return ok;
}
