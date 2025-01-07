#include "mqtt_client.h"
#include "servo_control.h"

extern DeviceStatus deviceStatus;
extern WiFiCredentials credentials;

MQTTClientManager mqttManager;

void MQTTClientManager::begin()
{
    mqttClient.setClient(espClient);
    setupMQTT();
}

void MQTTClientManager::setupMQTT()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(callback);
}

void MQTTClientManager::callback(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.println("收到MQTT消息: " + message);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.println("解析JSON失败");
        return;
    }

    MQTTCommand cmd;
    cmd.command = doc["command"].as<String>();
    cmd.position = doc["position"].as<int>();
    cmd.restore = doc["restore"].as<int>();
    cmd.pwd = doc["pwd"].as<String>();

    // 验证密码
    if (cmd.command != nullptr && cmd.command != "" && cmd.command != "null")
    {
        if (cmd.pwd != String(credentials.password))
        {
            Serial.println("密码错误:" + cmd.pwd);
            return;
        }
    }

    // 处理命令
    if (cmd.command == "start")
    {
        servoController.setRunning(true);
        deviceStatus.isServoRunning = true;
    }
    else if (cmd.command == "stop")
    {
        servoController.setRunning(false);
        deviceStatus.isServoRunning = false;
    }
    else if (cmd.command == "position")
    {
        servoController.setRunning(false);
        deviceStatus.isServoRunning = false;

        int targetPosition = constrain(cmd.position, 0, 180);
        int lastPosition = deviceStatus.servoPosition;

        servoController.setPosition(targetPosition);

        if (cmd.restore == 1)
        {
            delay(ServoController::calculateMoveTime(lastPosition, targetPosition));
            servoController.setPosition(lastPosition);
            deviceStatus.servoPosition = lastPosition;
        }
        else
        {
            deviceStatus.servoPosition = targetPosition;
        }
    }
}

void MQTTClientManager::checkConnection()
{
    if (!mqttClient.connected() && deviceStatus.isWiFiConnected)
    {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL)
        {
            lastReconnectAttempt = now;

            String clientId = "esp32-servo-" + String(WiFi.macAddress());

            if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
            {
                Serial.println("MQTT broker connected");
                mqttClient.subscribe(MQTT_TOPIC);
                deviceStatus.mqttTopic = MQTT_TOPIC;
            }
        }
    }
}

void MQTTClientManager::publishStatus()
{
    if (!mqttClient.connected())
        return;

    unsigned long now = millis();
    if (now - lastPublishTime > PUBLISH_INTERVAL)
    {
        lastPublishTime = now;

        JsonDocument doc;
        doc["running"] = deviceStatus.isServoRunning;
        doc["position"] = deviceStatus.servoPosition;

        String status;
        serializeJson(doc, status);
        mqttClient.publish(deviceStatus.mqttTopic.c_str(), status.c_str());
    }
}

void MQTTClientManager::update()
{
    checkConnection();

    if (mqttClient.connected())
    {
        mqttClient.loop();
        publishStatus();
    }
}