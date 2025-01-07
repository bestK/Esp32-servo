#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <EEPROM.h>
#include "types.h"

class WiFiManager
{
private:
    unsigned long lastReconnectAttempt = 0;
    int reconnectAttempts = 0;
    static const int MAX_RECONNECT_ATTEMPTS = 3;

public:
    void begin();
    void update();
    bool connect();
    void setupAP();
    bool resetSettings();
    bool saveCredentials(const char *ssid, const char *password);
    bool reconnect();
    bool loadCredentials();
    void clearCredentials();
    void resetReconnectCount();
};

extern WiFiManager wifiManager;

#endif