#pragma once

// 引脚配置
#define RESET_PIN 0          // 重置按钮引脚
#define SERVO_PIN 9          // 舵机引脚
#define RESET_HOLD_TIME 3000 // 重置按钮按下时间

// MQTT配置
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TOPIC "esp32/servo"
#define MQTT_USERNAME "emqx"
#define MQTT_PASSWORD "public"

// AP模式配置
#define AP_SSID "ESP32_Servo"
#define AP_PASSWORD "12345678"

// LED配置
#define LED_PIN 48
#define LED_COUNT 1
#define LED_BLINK_TIME 500
#define LED_BLINK_COLOR_RED 0xFF0000    // 红色
#define LED_BLINK_COLOR_GREEN 0x00FF00  // 绿色
#define LED_BLINK_COLOR_BLUE 0x0000FF   // 蓝色
#define LED_BLINK_COLOR_YELLOW 0xFFFF00 // 黄色
#define LED_BLINK_COLOR_PURPLE 0xFF00FF // 紫色
#define LED_BLINK_COLOR_CYAN 0x00FFFF   // 青色
#define LED_BLINK_COLOR_WHITE 0xFFFFFF  // 白色

#define STATUS_AP "AP"
#define STATUS_WIFI_CONNECTED "WIFI_CONNECTED"
#define STATUS_WIFI_DISCONNECTED "WIFI_DISCONNECTED"
#define STATUS_WIFI_CONNECTING "WIFI_CONNECTING"
#define STATUS_WIFI_ERROR "WIFI_ERROR"
#define STATUS_SERVO_RUNNING "SERVO_RUNNING"
#define STATUS_SERVO_STOPPED "SERVO_STOPPED"
