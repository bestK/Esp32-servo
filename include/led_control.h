#pragma once
#include <Adafruit_NeoPixel.h>

class LEDController
{
public:
    void begin();
    void blink(uint32_t color);
    void light(uint32_t color);
    void changeStatus(String status);

private:
    unsigned long lastBlinkTime = 0;
    bool blinkState = false;
    Adafruit_NeoPixel pixels;
};

extern LEDController ledController;