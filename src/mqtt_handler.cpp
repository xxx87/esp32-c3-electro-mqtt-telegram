#include "mqtt_handler.h"
#include "config.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure mqttSecClient;
static PubSubClient mqttClient(mqttSecClient);

void mqttSetup() {
  mqttSecClient.setInsecure();
  mqttClient.setServer(mqtt_server, MQTT_PORT);
}

void mqttLoop() {
  if (!mqttClient.connected() && strlen(mqtt_server) > 0) {
    mqttClient.connect(hostname, mqtt_user, mqtt_pass);
  }
  mqttClient.loop();
}

bool mqttIsConnected() {
  return mqttClient.connected();
}

bool mqttPublishPower(bool powerOn) {
  if (!mqttClient.connected() || strlen(mqtt_topic) == 0) {
    return false;
  }
  return mqttClient.publish(mqtt_topic, powerOn ? "ON" : "OFF", true);
}
