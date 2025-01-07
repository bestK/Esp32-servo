#pragma once
#include <PubSubClient.h>
#include <WiFi.h>
#include "types.h"
#include "config.h"

class MQTTClientManager
{
public:
    void begin();
    void update();
    bool isConnected() { return mqttClient.connected(); }

private:
    bool setupMQTT();
    void checkConnection();
    void publishStatus();
    static void callback(char *topic, byte *payload, unsigned int length);

    WiFiClient espClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt = 0;
    unsigned long lastPublishTime = 0;
    static const unsigned long RECONNECT_INTERVAL = 5000;
    static const unsigned long PUBLISH_INTERVAL = 5000;
};

extern MQTTClientManager mqttManager;