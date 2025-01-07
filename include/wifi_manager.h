#pragma once
#include <WiFi.h>
#include <EEPROM.h>
#include "types.h"
#include "config.h"

class WiFiManager
{
public:
    void begin();
    void update();
    bool connect();
    void setupAP();
    bool resetSettings();
    bool saveCredentials(const char *ssid, const char *password);
    bool reconnect();

    // Getters
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    String getSSID() { return WiFi.SSID(); }
    String getIP() { return WiFi.localIP().toString(); }

private:
    static const int WIFI_TIMEOUT = 20;      // 连接超时时间(秒)
    static const int WIFI_RETRY_DELAY = 500; // 重试延迟(毫秒)

    bool loadCredentials();
    void clearCredentials();
    unsigned long lastReconnectAttempt = 0;
    static const unsigned long RECONNECT_INTERVAL = 30000; // 30秒重连间隔
};

extern WiFiManager wifiManager;