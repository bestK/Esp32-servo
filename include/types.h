#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

struct Response
{
    bool success;
    JsonDocument data;
    String message;
    String toJson();
};

struct DeviceStatus
{
    bool isServoRunning;
    int servoPosition;
    bool isWiFiConnected;
    float voltage;
    String wifiSSID;
    String wifiIP;
    String wifiPasswd;
    int servoSpeed;
    String mqttTopic;
};

struct WiFiCredentials
{
    char ssid[32];
    char password[64];
};

struct MQTTCommand
{
    String command;
    int position;
    int restore;
    String pwd;
};