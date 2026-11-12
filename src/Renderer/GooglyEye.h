// * Source: https://github.com/adafruit/Adafruit_Learning_System_Guides/blob/main/Hallowing_Googly_Eye/Hallowing_Googly_Eye.ino
// SPDX-FileCopyrightText: 2018 Phillip Burgess for Adafruit Industries
// SPDX-License-Identifier: MIT
#pragma once

#include "GooglyEyeConfig.h"

class GooglyEye {
public:
    GooglyEye() : x(0), y(0), lastTime(0), vx(0), vy(0) {}
    float x, y;
    void renderEye(float accX, float accY) {
        // Orient the sensor directions to the display directions
        update(accX, accY);
    }

private:
    unsigned long lastTime;
    float vx, vy;

    void update(float inputX, float inputY);
};

