#include "web_server.h"
#include "led_control.h"
#include "servo_control.h"

String Response::toJson()
{
    JsonDocument doc;
    doc["success"] = success;
    doc["data"] = data;
    doc["message"] = message;
    String json;
    serializeJson(doc, json);
    return json;
}

extern WebServer server;
extern DeviceStatus deviceStatus;
extern WiFiCredentials credentials;

WebServerManager webServerManager;

// HTML页面
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32舵机控制</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 0; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .card { background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 20px; }
        .btn { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; }
        .btn:disabled { background: #ccc; }
        input[type="text"], input[type="password"] { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ddd; border-radius: 4px; }
        .status { margin-top: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h2>设备状态</h2>
            <div id="status" class="status">
                <p>WiFi: <span id="wifiStatus">%s</span></p>
                <p>IP: <span id="ipAddress">%s</span></p>
                <p>舵机位置: <span id="servoPos">%d°</span></p>
                <p>运行状态: <span id="runningStatus">%s</span></p>
            </div>
        </div>

        <div class="card">
            <h2>WiFi设置</h2>
            <form id="wifiForm">
                <input type="text" id="ssid" placeholder="WiFi名称" required><br>
                <input type="password" id="password" placeholder="WiFi密码" required><br>
                <button type="submit" class="btn">保存设置</button>
            </form>
        </div>

        <div class="card">
            <h2>舵机控制</h2>
            <button id="startBtn" class="btn">开始运行</button>
            <button id="stopBtn" class="btn">停止运行</button>
            <input type="range" id="posSlider" min="0" max="180" value="90">
            <span id="posValue">90°</span>
        </div>
    </div>

    <script>
        // 获取状态
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(res => {
                    const {data} = res;
                    document.getElementById('wifiStatus').textContent = data.wifi_ssid || '未连接';
                    document.getElementById('ipAddress').textContent = data.wifi_ip || '-';
                    document.getElementById('servoPos').textContent = data.servo_position + '°';
                    document.getElementById('runningStatus').textContent = data.is_running ? '运行中' : '已停止';
                });
        }

        // WiFi设置
        document.getElementById('wifiForm').onsubmit = function(e) {
            e.preventDefault();
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            fetch('/setwifi', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({ssid, password})
            })
            .then(response => response.json())
            .then(data => {
                alert(data.message);
                if(data.success) {
                    setTimeout(updateStatus, 5000);
                }
            });
        };

        // 舵机控制
        document.getElementById('startBtn').onclick = () => {
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: 'start'})
            });
        };

        document.getElementById('stopBtn').onclick = () => {
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: 'stop'})
            });
        };

        const slider = document.getElementById('posSlider');
        const posValue = document.getElementById('posValue');
        slider.oninput = function() {
            posValue.textContent = this.value + '°';
        };
        slider.onchange = function() {
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    command: 'position',
                    position: parseInt(this.value)
                })
            });
        };

        // 定期更新状态
        setInterval(updateStatus, 1000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

void WebServerManager::begin()
{
    setupRoutes();
    server.begin();
}

void WebServerManager::handleClient()
{
    server.handleClient();
}

void WebServerManager::setupRoutes()
{
    server.on("/", HTTP_GET, [this]()
              { handleRoot(); });
    server.on("/status", HTTP_GET, [this]()
              { handleStatus(); });
    server.on("/setwifi", HTTP_POST, [this]()
              { handleSetWiFi(); });
    server.on("/resetwifi", HTTP_POST, [this]()
              { handleResetWiFi(); });

    server.on("/control", HTTP_POST, [this]()
              { handleControl(); });
    server.onNotFound([this]()
                      { handleNotFound(); });
}

void WebServerManager::handleRoot()
{
    String html = String(INDEX_HTML);
    const char *runningStatus = deviceStatus.isServoRunning ? "运行中" : "已停止";
    String ipAddress = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "-";
    String wifiName = deviceStatus.wifiSSID[0] != '\0' ? deviceStatus.wifiSSID : "未连接";

    html.replace("%s", wifiName);
    html.replace("%s", ipAddress);
    html.replace("%d", String(deviceStatus.servoPosition));
    html.replace("%s", runningStatus);

    server.send(200, "text/html", html);
}

void WebServerManager::handleStatus()
{
    Response response;
    response.success = true;

    JsonDocument &data = response.data;
    data["wifi_ssid"] = deviceStatus.wifiSSID[0] != '\0' ? deviceStatus.wifiSSID : "未连接";
    data["wifi_ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "-";
    data["is_running"] = deviceStatus.isServoRunning;
    data["servo_position"] = deviceStatus.servoPosition;

    sendResponse(response);
}

void WebServerManager::handleSetWiFi()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"无效的请求\"}");
        return;
    }

    String json = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"解析JSON失败\"}");
        return;
    }

    const char *ssid = doc["ssid"];
    const char *password = doc["password"];

    if (!ssid || !password)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"缺少必要参数\"}");
        return;
    }

    // 保存WiFi凭证
    strncpy(credentials.ssid, ssid, sizeof(credentials.ssid) - 1);
    strncpy(credentials.password, password, sizeof(credentials.password) - 1);
    EEPROM.put(0, credentials);
    EEPROM.commit();

    Response response;
    response.success = true;
    response.message = "WiFi设置已保存，设备将尝试连接新的网络";
    sendResponse(response);

    // 延迟重启
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleResetWiFi()
{
    memset(&credentials, 0, sizeof(WiFiCredentials));
    EEPROM.put(0, credentials);
    EEPROM.commit();

    Response response;
    response.success = true;
    response.message = "WiFi设置已重置，设备将重启";
    sendResponse(response);

    delay(1000);
    ESP.restart();
}

void WebServerManager::handleControl()
{

    String json = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"解析JSON失败\"}");
        return;
    }

    String command = doc["command"];
    int position = doc["position"];
    int restore = doc["restore"];

    if (command == "start")
    {
        servoController.setRunning(true);
        deviceStatus.isServoRunning = true;
        ledController.changeStatus(STATUS_SERVO_RUNNING);
    }
    else if (command == "stop")
    {
        servoController.setRunning(false);
        deviceStatus.isServoRunning = false;
        ledController.changeStatus(STATUS_SERVO_STOPPED);
    }
    else if (command == "position")
    {
        String lastStatus = ledController.getCurrentStatus();
        ledController.changeStatus(STATUS_MQTT_RECEIVE);

        int targetPosition = constrain(position, 0, 180);
        int lastPosition = deviceStatus.servoPosition;
        servoController.setPosition(targetPosition);
        deviceStatus.servoPosition = targetPosition;
        if (restore == 1)
        {
            delay(ServoController::calculateMoveTime(deviceStatus.servoPosition, targetPosition));
            servoController.setPosition(deviceStatus.servoPosition);
            deviceStatus.servoPosition = deviceStatus.servoPosition;
        }
        else
        {
            deviceStatus.servoPosition = targetPosition;
        }
        ledController.changeStatus(lastStatus);
    }

    Response response;
    response.success = true;
    response.message = "控制命令已接收";
    sendResponse(response);
}

void WebServerManager::handleNotFound()
{
    Response response;
    response.success = false;
    response.message = "404: Not Found";
    sendResponse(response);
}

void WebServerManager::sendResponse(Response &response)
{
    server.send(200, "application/json", response.toJson());
}

String WebServerManager::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".json"))
        return "application/json";
    return "text/plain";
}