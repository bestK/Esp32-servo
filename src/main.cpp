#include "config.h"
#include "types.h"
#include "led_control.h"
#include "servo_control.h"
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <web_server.h>
#include <mqtt_client.h>
#include "wifi_manager.h"

// 全局变量
DeviceStatus deviceStatus;
WiFiCredentials credentials;
WebServer server(80);
extern WebServerManager webServerManager;

// WiFi重连相关变量
static unsigned long lastWiFiReconnectAttempt = 0;

// 按键相关变量和常量
const unsigned long SHORT_PRESS_TIME = 1000; // 1秒内的按压被认为是短按
static unsigned long btnPressTime = 0;
static bool btnPressed = false;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(sizeof(WiFiCredentials));
  // 初始化所有管理器
  /**
   * Adafruit_NeoPixel 库在初始化时会配置 RMT（Remote Control）通道，这是 ESP32 的一个硬件特性
   * ESP32Servo 库也需要使用 RMT 通道来生成 PWM 信号控制舵机
   * 当 LED 控制器先初始化时，可能会占用舵机所需的 RMT 通道，导致舵机初始化时出现资源冲突
   * 所以 舵机控制器的初始化放在前面
   */
  servoController.begin(SERVO_PIN);
  ledController.begin();
  wifiManager.begin();
  mqttManager.begin();
  webServerManager.begin();

  // 设置按键引脚
  pinMode(RESET_PIN, INPUT_PULLUP);

  // 尝试连接WiFi或启动AP模式
  if (!wifiManager.connect())
  {
    wifiManager.setupAP();
  }
}

void loop()
{
  // 更新所有管理器状态
  wifiManager.update();
  servoController.update();
  mqttManager.update();
  webServerManager.handleClient();

  // LED状态更新
  if (WiFi.status() == WL_CONNECTED)
  {
    if (deviceStatus.isServoRunning)
    {
      ledController.changeStatus(STATUS_SERVO_RUNNING);
    }
    else
    {
      ledController.changeStatus(STATUS_WIFI_CONNECTED);
    }
  }

  // 按键检测
  int btnState = digitalRead(RESET_PIN);
  if (btnState == LOW && !btnPressed)
  {
    btnPressed = true;
    btnPressTime = millis();
  }

  if (btnState == HIGH && btnPressed)
  {
    unsigned long pressDuration = millis() - btnPressTime;
    if (pressDuration < SHORT_PRESS_TIME)
    {
      wifiManager.reconnect();
    }
    btnPressed = false;
  }
}
