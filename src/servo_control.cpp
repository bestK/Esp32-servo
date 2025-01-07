#include "servo_control.h"

ServoController servoController;

void ServoController::begin(int pin)
{
    _servo.attach(pin);
    _isRunning = false;
    _currentPosition = 0;
}

void ServoController::setRunning(bool running)
{
    _isRunning = running;
}

void ServoController::setPosition(int position)
{
    // 添加位置范围检查
    if (position < 0 || position > 180)
    {
        return;
    }
    _currentPosition = position;
    _servo.write(position);
    Serial.print("舵机移动到位置 setPosition: ");
    Serial.println(position);
}

void ServoController::update()
{
    if (!_isRunning)
    {
        return;
    }

    unsigned long currentMillis = millis();
    static unsigned long previousMillis = 0;
    const long interval = 2000; // 每2秒切换一次位置

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        // 计算目标位置
        int targetPosition = _currentPosition == 0 ? 180 : 0;

        // 设置新位置
        _servo.write(targetPosition);
        _currentPosition = targetPosition;

        // 不使用delay，改用millis()实现非阻塞延时

        Serial.print("舵机移动到位置: ");
        Serial.println(targetPosition);
    }
}