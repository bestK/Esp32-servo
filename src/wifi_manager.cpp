#include "wifi_manager.h"
#include "led_control.h"
#include <types.h>
#include <config.h>

extern DeviceStatus deviceStatus;
extern WiFiCredentials credentials;

WiFiManager wifiManager;

bool isAPMode = false;

void WiFiManager::begin()
{
    WiFi.mode(WIFI_STA);
    loadCredentials();
}

void WiFiManager::update()
{
    if (isAPMode)
    {
        return; // AP模式下不再尝试重连
    }

    static unsigned long lastCheck = 0;
    unsigned long now = millis();

    if (now - lastCheck > 5000)
    {
        lastCheck = now;
        if (WiFi.status() != WL_CONNECTED)
        {
            if (deviceStatus.isWiFiConnected)
            {
                Serial.println("WiFi断开，尝试重新连接...");
                deviceStatus.isWiFiConnected = false;
                ledController.changeStatus(STATUS_WIFI_DISCONNECTED);
            }
            WiFi.disconnect();
            connect();
        }
        else if (!deviceStatus.isWiFiConnected)
        {
            deviceStatus.isWiFiConnected = true;
            deviceStatus.wifiIP = WiFi.localIP().toString();
            ledController.changeStatus(STATUS_WIFI_CONNECTED);
        }
    }
}

bool WiFiManager::connect()
{
    if (strlen(credentials.ssid) == 0 || strlen(credentials.password) == 0)
    {
        Serial.println("没有保存的WiFi凭证");
        Serial.println("ssid: " + String(credentials.ssid));
        Serial.println("password: " + String(credentials.password));
        return false;
    }

    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS)
    {
        Serial.println("已达到最大重连次数");
        ledController.changeStatus(STATUS_WIFI_ERROR);
        Serial.println("尝试启动AP模式");
        setupAP();
        return true;
    }

    Serial.println("尝试连接WiFi...");
    Serial.print("SSID: ");
    Serial.println(credentials.ssid);
    Serial.print("密码: ");
    Serial.println(credentials.password);
    Serial.print("重连次数: ");
    Serial.println(reconnectAttempts + 1);

    ledController.changeStatus(STATUS_WIFI_CONNECTING);
    WiFi.begin(credentials.ssid, credentials.password);

    // 等待连接建立
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) // 20次尝试，每次500ms
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi连接成功!");
        Serial.print("IP地址: ");
        Serial.println(WiFi.localIP());
        deviceStatus.wifiIP = WiFi.localIP().toString();
        deviceStatus.wifiSSID = credentials.ssid;
        deviceStatus.isWiFiConnected = true;
        ledController.changeStatus(STATUS_WIFI_CONNECTED);
        resetReconnectCount();
        return true;
    }

    reconnectAttempts++;
    return false;
}

void WiFiManager::setupAP()
{
    isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.println("AP模式已启动");
    Serial.print("AP IP地址: ");
    Serial.println(WiFi.softAPIP());
    ledController.changeStatus(STATUS_AP); // AP模式
}

bool WiFiManager::resetSettings()
{
    clearCredentials();
    WiFi.disconnect(true);
    return true;
}

bool WiFiManager::saveCredentials(const char *ssid, const char *password)
{
    if (!ssid || !password)
        return false;

    strncpy(credentials.ssid, ssid, sizeof(credentials.ssid) - 1);
    strncpy(credentials.password, password, sizeof(credentials.password) - 1);

    EEPROM.put(0, credentials);
    bool success = EEPROM.commit();

    if (success)
    {
        Serial.println("WiFi凭证已保存");
        deviceStatus.wifiSSID = ssid;
        deviceStatus.wifiPasswd = password;
    }

    return success;
}

bool WiFiManager::reconnect()
{
    lastReconnectAttempt = millis();

    if (strlen(credentials.ssid) == 0)
    {
        return false;
    }

    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS)
    {
        Serial.println("已达到最大重连次数");
        return false;
    }

    Serial.println("尝试重新连接WiFi...");
    WiFi.disconnect();
    delay(1000);
    return connect();
}

bool WiFiManager::loadCredentials()
{
    EEPROM.get(0, credentials);
    return (strlen(credentials.ssid) > 0);
}

void WiFiManager::clearCredentials()
{
    memset(&credentials, 0, sizeof(WiFiCredentials));
    EEPROM.put(0, credentials);
    EEPROM.commit();
}

void WiFiManager::resetReconnectCount()
{
    reconnectAttempts = 0;
}