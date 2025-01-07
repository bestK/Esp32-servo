#include "mqtt_client.h"
#include "servo_control.h"
#include <led_control.h>

extern DeviceStatus deviceStatus;
extern WiFiCredentials credentials;

MQTTClientManager mqttManager;

void MQTTClientManager::begin()
{
    mqttClient.setClient(espClient);
    setupMQTT();
}

bool MQTTClientManager::setupMQTT()
{
    if (!deviceStatus.isWiFiConnected)
    {
        Serial.println("WiFi未连接，无法连接MQTT");
        return false;
    }

    // 检查WiFi连接状态
    Serial.print("WiFi状态: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());

    // 设置MQTT服务器
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(callback);
    mqttClient.setKeepAlive(MQTT_KEEPALIVE);

    // 生成唯一的客户端ID
    String client_id = "esp32-servo-" + String(WiFi.macAddress());
    Serial.print("MQTT客户端ID: ");
    Serial.println(client_id);

    // 尝试连接MQTT服务器
    if (mqttClient.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD, nullptr, 0, true, nullptr, 0))
    {
        Serial.println("MQTT连接成功");

        // 订阅主题并检查结果
        if (mqttClient.subscribe(MQTT_TOPIC))
        {
            Serial.println("成功订阅主题: " + String(MQTT_TOPIC));
            deviceStatus.mqttTopic = MQTT_TOPIC;
        }
        else
        {
            Serial.println("订阅主题失败");
        }
        return true;
    }

    // 输出详细的错误信息
    int state = mqttClient.state();
    Serial.print("MQTT连接失败，错误码: ");
    Serial.print(state);
    Serial.print(" - ");
    switch (state)
    {
    case -4:
        Serial.println("MQTT_CONNECTION_TIMEOUT");
        break;
    case -3:
        Serial.println("MQTT_CONNECTION_LOST");
        break;
    case -2:
        Serial.println("MQTT_CONNECT_FAILED");
        break;
    case -1:
        Serial.println("MQTT_DISCONNECTED");
        break;
    case 1:
        Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
        break;
    case 2:
        Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
        break;
    case 3:
        Serial.println("MQTT_CONNECT_UNAVAILABLE");
        break;
    case 4:
        Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
        break;
    case 5:
        Serial.println("MQTT_CONNECT_UNAUTHORIZED");
        break;
    default:
        Serial.println("未知错误");
    }
    return false;
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
        ledController.changeStatus(STATUS_SERVO_RUNNING);
    }
    else if (cmd.command == "stop")
    {
        servoController.setRunning(false);
        deviceStatus.isServoRunning = false;
        ledController.changeStatus(STATUS_SERVO_STOPPED);
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
    if (deviceStatus.isWiFiConnected && !mqttClient.connected())
    {
        Serial.println("MQTT未连接，尝试重新连接");
        setupMQTT();
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