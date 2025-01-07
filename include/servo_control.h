#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <ESP32Servo.h>

class ServoController
{
private:
    Servo _servo;
    bool _isRunning;
    int _currentPosition;

public:
    void begin(int pin);
    void setRunning(bool running);
    void setPosition(int position);
    void update();
    int getCurrentPosition() const { return _currentPosition; }

    static int calculateMoveTime(int fromPos, int toPos)
    {
        int angleDiff = abs(toPos - fromPos);
        return (angleDiff * 100 / 60) + 500; // 假设60度/0.1秒
    }
};

extern ServoController servoController;

#endif // SERVO_CONTROL_H