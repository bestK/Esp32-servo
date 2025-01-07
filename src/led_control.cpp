#include <Adafruit_NeoPixel.h>
#include "led_control.h"
#include "config.h"

LEDController ledController;

void LEDController::begin()
{
    pixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
    pixels.begin();
    pixels.setBrightness(50);
}

String LEDController::getCurrentStatus()
{
    return _currentStatus;
}

void LEDController::changeStatus(String status)
{
    _currentStatus = status;
    if (status == STATUS_AP)
    {
        blink(0); // 黑色闪烁
    }
    else if (status == STATUS_WIFI_DISCONNECTED)
    {
        blink(LED_BLINK_COLOR_WHITE); // 白色闪烁
    }
    else if (status == STATUS_WIFI_CONNECTING)
    {
        blink(LED_BLINK_COLOR_YELLOW); // 黄色闪烁
    }
    else if (status == STATUS_WIFI_ERROR)
    {
        light(LED_BLINK_COLOR_YELLOW); // 黄色常亮
    }
    else if (status == STATUS_WIFI_CONNECTED)
    {
        blink(LED_BLINK_COLOR_GREEN); // 绿色闪烁
    }
    else if (status == STATUS_SERVO_RUNNING)
    {
        blink(LED_BLINK_COLOR_BLUE); // 蓝色闪烁
    }
    else if (status == STATUS_MQTT_RECEIVE)
    {
        blink(LED_BLINK_COLOR_PURPLE); // 紫色闪烁
    }
    else if (status == STATUS_SERVO_STOPPED)
    {
        blink(LED_BLINK_COLOR_RED); // 红色闪烁
    }
}

void LEDController::blink(uint32_t color)
{
    unsigned long currentTime = millis();

    if (currentTime - lastBlinkTime >= 200)
    {
        lastBlinkTime = currentTime;
        blinkState = !blinkState;

        if (blinkState)
        {
            pixels.setPixelColor(0, color);
        }
        else
        {
            pixels.setPixelColor(0, 0);
        }
        pixels.show();
    }
}

void LEDController::light(uint32_t color)
{
    pixels.setPixelColor(0, color);
    pixels.show();
}

void LEDController::update()
{
    changeStatus(_currentStatus);
}