#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

struct Response
{
  bool success;
  JsonDocument data;
  String message;
  String toJson();
};

String Response::toJson()
{
  JsonDocument doc;
  doc["success"] = success;
  doc["message"] = message;
  doc["data"] = data;
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

// 定义状态数据结构
struct DeviceStatus
{
  bool isServoRunning;  // 舵机运行状态
  int servoPosition;    // 舵机当前位置
  bool isWiFiConnected; // WiFi连接状态
  float voltage;        // 电压值（如果需要）
  String wifiSSID;      // 当前连接的WiFi名称
  String wifiIP;        // 当前连接的WiFiIP
  String wifiPasswd;    // 当前连接的WiFi密码
  int servoSpeed;       // 舵机运行速度 (1-100)
};

DeviceStatus deviceStatus; // 全局变量声明
// WS2812 LED配置
#define LED_PIN 48  // WS2812 数据引脚
#define LED_COUNT 1 // LED数量
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int RESET_PIN = 0;          // BOOT按钮
const int SERVO_PIN = 9;          // 舵机引脚
const int RESET_HOLD_TIME = 3000; // 长按3秒触发重置

Servo myservo;
WebServer server(80);

// AP模式的配置
const char *apSSID = "ESP32_Servo";  // ESP32创建的热点名称
const char *apPassword = "12345678"; // 热点密码

// 存储WiFi信息
struct WiFiCredentials
{
  char ssid[32];
  char password[64];
} credentials;

// 配置页面HTML
const char *configHTML = R"rawstr(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>WiFi配置</title>
        <style>
            body {
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                padding: 20px;
                max-width: 500px;
                margin: 0 auto;
                background: #f5f5f5;
            }
            .container {
                background: white;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            }
            h2 {
                color: #333;
                margin-bottom: 20px;
            }
            .form-group {
                margin-bottom: 15px;
            }
            label {
                display: block;
                margin-bottom: 5px;
                color: #666;
            }
            input[type='text'],
            input[type='password'] {
                width: 100%;
                padding: 8px;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-sizing: border-box;
            }
            .submit-btn {
                background: #4caf50;
                color: white;
                border: none;
                padding: 10px 20px;
                border-radius: 4px;
                cursor: pointer;
                width: 100%;
                margin-top: 10px;
            }
            .reset-btn {
                background: #ff4444;
                color: white;
                border: none;
                padding: 10px 20px;
                border-radius: 4px;
                cursor: pointer;
                width: 100%;
                margin-top: 20px;
            }
            .submit-btn:hover {
                background: #45a049;
            }
            .reset-btn:hover {
                background: #ff3333;
            }
            .password-container {
                position: relative;
                width: 100%;
            }
            .password-toggle {
                position: absolute;
                right: 10px;
                top: 50%;
                transform: translateY(-50%);
                cursor: pointer;
                user-select: none;
                color: #666;
            }
        </style>

        <script>
            function togglePassword() {
                const passwordInput = document.getElementById('password');
                const toggleBtn = document.querySelector('.password-toggle');

                if (passwordInput.type === 'password') {
                    passwordInput.type = 'text';
                    toggleBtn.textContent = '🙈';
                } else {
                    passwordInput.type = 'password';
                    toggleBtn.textContent = '👀';
                }
            }
        </script>
    </head>
    <body>
        <div class="container">
            <h2>WiFi配置</h2>
            <form method="post" action="/api/wifi/save">
                <div class="form-group">
                    <label for="ssid">WiFi名称</label>
                    <input type="text" id="ssid" name="ssid" value="%s" required />
                </div>
                <div class="form-group">
                    <label for="password">WiFi密码</label>
                    <div class="password-container">
                        <input type="password" id="password" name="password" value="%s" required />
                        <span class="password-toggle" onclick="togglePassword()">👀</span>
                    </div>
                </div>
                <button type="submit" class="submit-btn">保存并连接</button>
            </form>
            <form method="post" action="/api/wifi/reset">
                <button type="submit" class="reset-btn">重置设备</button>
            </form>
        </div>
    </body>
</html>

)rawstr";

// 添加全局变量用于LED控制
static unsigned long lastBlinkTime = 0;
static bool blinkState = false;
static int currentBlinkColor = 0;

void ledBlink(int color)
{
  unsigned long currentTime = millis();

  // 每400ms切换一次LED状态
  if (currentTime - lastBlinkTime >= 400)
  {
    lastBlinkTime = currentTime;
    blinkState = !blinkState;
    currentBlinkColor = color;

    if (blinkState)
    {
      pixels.setPixelColor(0, color);
    }
    else
    {
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    }
    pixels.show();
  }
}

void ledStatus(int status)
{
  pixels.clear();
  switch (status)
  {
  case 0: // AP模式 - 蓝色闪烁
    ledBlink(pixels.Color(0, 0, 255));
    break;
  case 1: // WiFi连接中 - 黄色闪烁
    ledBlink(pixels.Color(255, 255, 0));
    break;
  case 2: // WiFi已连接 - 绿色常亮
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    break;
  case 3: // 舵机运行 - 绿色闪烁
    ledBlink(pixels.Color(0, 255, 0));
    break;
  default: // 关闭
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void resetWiFiSettings()
{
  // 清除EEPROM中的WiFi配置
  memset(&credentials, 0, sizeof(credentials));
  EEPROM.put(0, credentials);
  EEPROM.commit();
  Serial.println("WiFi配置已重置");
  delay(1000);
  ESP.restart();
}

void setupAP()
{
  WiFi.softAP(apSSID, apPassword);
  Serial.println("AP模式已启动");
  Serial.print("IP地址: ");
  Serial.println(WiFi.softAPIP());
}

void setupServer()
{
  // 配置页面
  server.on("/", HTTP_GET, []()
            {
    char html[4096];
    snprintf(html, sizeof(html), configHTML, 
             credentials.ssid,     // 填充SSID
             credentials.password  // 填充密码
    );
    server.send(200, "text/html", html); });

  // 保存WiFi配置
  server.on("/api/wifi/save", HTTP_POST, []()
            {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    strncpy(credentials.ssid, ssid.c_str(), sizeof(credentials.ssid));
    strncpy(credentials.password, password.c_str(), sizeof(credentials.password));
    EEPROM.put(0, credentials);
    EEPROM.commit();
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    Response response;
    response.success = true;
    response.message = "WiFi配置已保存，设备将重启...";
    server.send(200, "application/json", response.toJson());
    delay(2000);
    ESP.restart(); });

  // 重置路由
  server.on("/api/wifi/reset", HTTP_POST, []()
            {
    Response response;
    response.success = true;
    response.message = "设备正在重置并重启...";
    server.send(200, "application/json", response.toJson());
    delay(1000);
    resetWiFiSettings(); });

  // 舵机控制API
  server.on("/api/servo/start", HTTP_GET, []()
            {
    deviceStatus.isServoRunning = true;
    Response response;
    response.success = true;
    response.message = "舵机开始运行";
    server.send(200, "application/json", response.toJson()); });

  server.on("/api/servo/stop", HTTP_GET, []()
            {
    deviceStatus.isServoRunning = false;
    Response response;
    response.success = true;
    response.message = "舵机停止运行";
    server.send(200, "application/json", response.toJson()); });

  // 获取设备状态
  server.on("/api/status", HTTP_GET, []()
            {
  
      JsonDocument doc; 

      // 创建嵌套对象的正确方式
      JsonObject servo = doc["servo"].to<JsonObject>();
      servo["running"] = deviceStatus.isServoRunning;
      servo["position"] = deviceStatus.servoPosition;

      JsonObject wifi = doc["wifi"].to<JsonObject>();
      wifi["connected"] = deviceStatus.isWiFiConnected;
      wifi["ssid"] = deviceStatus.wifiSSID;

      doc["voltage"] = deviceStatus.voltage;

      Response response;
      response.success = true;
      response.data = doc;
      response.message = "设备信息";
      server.send(200, "application/json", response.toJson()); });

  server.on("/api/servo/position", HTTP_GET, []()
            {
    if (server.hasArg("position")) {
        int targetPosition = server.arg("position").toInt();
        targetPosition = constrain(targetPosition, 0, 180);
        int lastPosition = deviceStatus.servoPosition;
        
        // 移动到目标位置
        myservo.write(targetPosition);
        
        if (server.hasArg("restore") && server.arg("restore").toInt() == 1) {
            // 根据角度差计算所需时间
            int angleDiff = abs(targetPosition - lastPosition);
            // 假设舵机速度约为60度/0.1秒，再加上一些余量
            int moveTime = (angleDiff * 100 / 60) + 500; // 单位：毫秒
            
            delay(moveTime);
            myservo.write(lastPosition);
            deviceStatus.servoPosition = lastPosition;
        } else {
            deviceStatus.servoPosition = targetPosition;
        }

        Response response;
        response.success = true;
        response.message = "舵机位置已更新";
        server.send(200, "application/json", response.toJson());
    } else {
        Response response;
        response.success = false;
        response.message = "缺少position参数";
        server.send(400, "application/json", response.toJson());
    } });

  server.begin();
}

bool connectSavedWiFi()
{
  EEPROM.get(0, credentials);
  if (strlen(credentials.ssid) > 0)
  {
    Serial.println("尝试连接保存的WiFi...");
    Serial.println(credentials.ssid);
    Serial.println(credentials.password);
    ledStatus(1);
    WiFi.begin(credentials.ssid, credentials.password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nWiFi已连接");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());
      return true;
    }
  }
  return false;
}

// 添加按键相关变量
const int RESET_BTN_PIN = 0;                 // Reset按键引脚，通常是GPIO0
const unsigned long SHORT_PRESS_TIME = 1000; // 短按定义为1秒内
static unsigned long btnPressTime = 0;
static bool btnPressed = false;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(sizeof(WiFiCredentials));

  // 初始化WS2812
  pixels.begin();
  pixels.setBrightness(50); // 设置亮度 (0-255)
  pixels.clear();
  pixels.show();

  pinMode(RESET_PIN, INPUT_PULLUP);
  myservo.attach(SERVO_PIN);

  if (credentials.ssid != "" && credentials.password != "" && !connectSavedWiFi())
  {
    setupAP();
    deviceStatus.isWiFiConnected = false;
    ledStatus(0); // AP模式快闪
  }
  else if (credentials.ssid != "" && credentials.password != "")
  {
    deviceStatus.wifiSSID = credentials.ssid;
    deviceStatus.wifiPasswd = credentials.password;
    if (WiFi.status() == WL_CONNECTED)
    {
      deviceStatus.isWiFiConnected = true;
      deviceStatus.wifiIP = WiFi.localIP().toString();
    }
  }

  setupServer();
  pinMode(RESET_BTN_PIN, INPUT_PULLUP); // 设置Reset按键为输入上拉
}

void loop()
{
  server.handleClient();

  // LED状态更新
  if (deviceStatus.isWiFiConnected)
  {
    if (deviceStatus.isServoRunning)
    {
      ledStatus(3); // 舵机运行时慢闪
    }
    else
    {
      ledStatus(2); // WiFi已连接常亮
    }
  }
  else
  {
    ledStatus(0); // AP模式快闪
  }

  // 检测重置按钮
  static unsigned long pressStartTime = 0;
  static bool buttonPressed = false;

  // 重置按钮检测时的LED效果修改
  if (digitalRead(RESET_PIN) == LOW)
  {
    if (!buttonPressed)
    {
      buttonPressed = true;
      pressStartTime = millis();
    }
    else if ((millis() - pressStartTime) > RESET_HOLD_TIME)
    {
      // 重置前红色闪烁3次
      for (int i = 0; i < 6; i++)
      {
        pixels.setPixelColor(0, (i % 2) ? pixels.Color(50, 0, 0) : pixels.Color(0, 0, 0));
        pixels.show();
        delay(100);
      }
      resetWiFiSettings();
    }
  }
  else
  {
    buttonPressed = false;
  }

  // 舵机控制
  static float currentPos = 90;          // 使用浮点数实现平滑过渡
  static float targetPos = 180;          // 目标位置
  static float moveSpeed = 0.8;          // 移动速度
  static unsigned long lastMoveTime = 0; // 上次移动时间

  if (deviceStatus.isServoRunning)
  {
    unsigned long currentTime = millis();
    // 每20ms才更新一次位置，减少抖动
    if (currentTime - lastMoveTime >= 20)
    {
      lastMoveTime = currentTime;

      if (abs(currentPos - targetPos) < moveSpeed)
      {
        // 到达目标位置，切换方向
        if (targetPos >= 180)
        {
          targetPos = 0;
        }
        else
        {
          targetPos = 180;
        }
      }

      // 平滑移动到目标位置
      if (currentPos < targetPos)
      {
        currentPos += moveSpeed;
      }
      else
      {
        currentPos -= moveSpeed;
      }

      // 只有当位置变化超过1度时才实际写入舵机
      static int lastWrittenPos = -1;
      int newPos = round(currentPos);
      if (newPos != lastWrittenPos)
      {
        myservo.write(newPos);
        lastWrittenPos = newPos;
      }
    }
    // 移除delay，使用时间控制来替代
  }

  // 在loop开始处添加按键检测
  int btnState = digitalRead(RESET_BTN_PIN);

  // 按下检测
  if (btnState == LOW && !btnPressed)
  { // 按键按下（低电平）
    btnPressed = true;
    btnPressTime = millis();
  }

  // 释放检测
  if (btnState == HIGH && btnPressed)
  { // 按键释放（高电平）
    unsigned long pressDuration = millis() - btnPressTime;
    if (pressDuration < SHORT_PRESS_TIME)
    { // 短按
      // 重新连接WiFi
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(credentials.ssid, credentials.password);

      Serial.println("正在重新连接WiFi...");
    }
    btnPressed = false;
  }
}
