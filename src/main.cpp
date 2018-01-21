#include <Arduino.h>
#include <ArduinoJson.h>
#include <MqttClient.h>
#include <FotaClient.h>
#include <ESPWifiClient.h>
#include <RemotePrint.h>
#include "settings.h"
#include "CommonConfig.h"
#include <ParadoxControlPanel/ParadoxControlPanel.h>

ESPWifiClient *wifiClient = new ESPWifiClient(WIFI_SSID, WIFI_PASS);

FotaClient *fotaClient            = new FotaClient(DEVICE_NAME);
ParadoxControlPanel *controlPanel =
  new ParadoxControlPanel(ALARM_MODULE_HOSTNAME,
                          ALARM_MODULE_PASSWORD,
                          ALARM_USER_PIN);


long lastAttempt       = 0;
MqttClient *mqttClient = NULL;

void publishStatus(const char *messageId = "", bool force = false, const char *areaName = "") {
  if (force == true) {
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2);
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root        = jsonBuffer.createObject();
    JsonObject& status      = root.createNestedObject("status");
    JsonArray & areasStatus = status.createNestedArray("areasStatus");
    JsonObject& area        = areasStatus.createNestedObject();
    area["name"]       = areaName;
    area["status"]     = 2;                                                 // armed
    area["statusName"] = ParadoxControlPanel::getAreaStatusFriendlyName(2); // armed

    if (messageId != NULL) {
      root["messageId"] = messageId;
    }

    String areasInfo = "";
    root.printTo(areasInfo);

    mqttClient->publish(MQTT_TOPIC_GET, areasInfo);
  } else {
    String areasInfo = controlPanel->getLatestAreasInfo();

    if (areasInfo.length() != 0) {
      mqttClient->publish(MQTT_TOPIC_GET, areasInfo);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  PRINT("MQTT Message arrived [");
  PRINT(topic);
  PRINTLN("] ");

  String payloadString = String((char *)payload);

  // Do something according the topic
  if (strcmp(topic, MQTT_TOPIC_SET) == 0) {
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 90;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root   = jsonBuffer.parseObject(payloadString);
    JsonObject& status = root.get<JsonObject&>("status");

    if (!status.success()) {
      PRINTLN("Paradox: JSON with \"status\" key not received.");
  #ifdef DEBUG_ENABLED
      root.prettyPrintTo(Serial);
  #endif // ifdef DEBUG_ENABLED
      return;
    }
    const char *armChar = status.get<const char *>("arm");

    if (armChar) {
      QueueItem item;
      item.areaName = armChar;
      item.action   = Action::armArea;
      controlPanel->queueActionAdd(item);

      const char *messageId = root.get<const char *>("messageId");
      publishStatus(messageId, true, armChar);

      // TODO: We need the loop here to make sure that the message will be sent asap
      mqttClient->loop();
    }
  }
  else {
    PRINT("MQTT: Warning: Unknown topic: ");
    PRINTLN(topic);
  }
}

void setup() {
  fotaClient->init();
  mqttClient = new MqttClient(MQTT_SERVER,
                              MQTT_SERVER_PORT,
                              DEVICE_NAME,
                              MQTT_USERNAME,
                              MQTT_PASS,
                              MQTT_TOPIC_SET,
                              MQTT_SERVER_FINGERPRINT,
                              mqttCallback);

  Serial.println(WIFI_SSID);
  wifiClient->init();
}

void getControlPanelStatus() {
  if  ((lastAttempt != 0) &&
       (millis() <= lastAttempt + MQTT_ALARM_PUBLISH_STATUS_INTERVAL)) {
    return;
  }
  QueueItem item;
  item.areaName = "";
  item.action   = Action::getStatus;
  controlPanel->queueActionAdd(item);
  lastAttempt = millis();
}

void loop() {
  RemotePrint::instance()->handle();
  fotaClient->loop();
  mqttClient->loop();
  getControlPanelStatus();
  publishStatus();
  controlPanel->process();
}
