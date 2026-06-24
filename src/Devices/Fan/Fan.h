#pragma once

#include <Arduino.h>
#include "config.h"

#if HAS_FAN_CONTROL

class Fan {
public:
    Fan();

    void setSpeed(int speed, int fadeSpeed = 10);
    int getSpeed() const;
    void update();

private:
    int currentSpeed = 0;      // Current speed (0-100)
    int targetSpeed = 0;       // Target speed (0-100)
    int transitionSpeed = 10;  // Fade speed per update
    float pwmValue = 0.0f;     // Actual PWM duty cycle (for smooth transitions)
};

#endif // HAS_FAN_CONTROL
