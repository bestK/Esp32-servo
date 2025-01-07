#include "wifi_manager.h"
#include "led_control.h"

extern DeviceStatus deviceStatus;
extern WiFiCredentials credentials;

WiFiManager wifiManager;

void WiFiManager::begin()
{
    WiFi.mode(WIFI_STA);
    loadCredentials();
}

void WiFiManager::update()
{
    static unsigned long lastAttemptTime = 0;

    if (!isConnected())
    {
        // 保持 CONNECTING 状态，让LED持续闪烁
        ledController.changeStatus(STATUS_WIFI_CONNECTING);

        if (millis() - lastAttemptTime > WIFI_RETRY_DELAY)
        {
            lastAttemptTime = millis();
            // 检查是否超时
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("WiFi连接成功");
                ledController.changeStatus(STATUS_WIFI_CONNECTED);
            }
        }
    }

    // 更新设备状态
    deviceStatus.isWiFiConnected = isConnected();
    if (isConnected())
    {
        deviceStatus.wifiSSID = getSSID();
        deviceStatus.wifiIP = getIP();
    }
    else
    {
        deviceStatus.wifiSSID = "";
        deviceStatus.wifiIP = "";
    }
}

bool WiFiManager::connect()
{
    if (strlen(credentials.ssid) == 0)
    {
        Serial.println("没有保存的WiFi凭证");
        return false;
    }

    Serial.println("尝试连接WiFi...");
    Serial.print("SSID: ");
    Serial.println(credentials.ssid);
    ledController.changeStatus(STATUS_WIFI_CONNECTING); // WiFi连接中
    WiFi.begin(credentials.ssid, credentials.password);

    return true;
}

void WiFiManager::setupAP()
{
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