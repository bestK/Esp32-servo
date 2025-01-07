#pragma once
#include <Adafruit_NeoPixel.h>

class LEDController
{
public:
    void begin();
    void blink(uint32_t color);
    void light(uint32_t color);
    void changeStatus(String status);
    void update();
    String getCurrentStatus();

private:
    unsigned long lastBlinkTime = 0;
    bool blinkState = false;
    Adafruit_NeoPixel pixels;
    String _currentStatus;
};

extern LEDController ledController;