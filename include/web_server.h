#pragma once
#include <WebServer.h>
#include <EEPROM.h>
#include "types.h"
#include "config.h"

class WebServerManager
{
public:
    void begin();
    void handleClient();

private:
    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleSetWiFi();
    void handleNotFound();
    void handleResetWiFi();
    String getContentType(String filename);
    void sendResponse(Response &response);
};

extern WebServerManager webServerManager;