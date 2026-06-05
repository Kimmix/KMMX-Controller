#include "HornLED.h"
#include "Utils/Utils.h"  // Use optimized utility functions

HornLED::HornLED() {
    ledcSetup(hornPwmChannel, hornFrequency, hornResolution);
    ledcAttachPin(LED_PWM_PIN, hornPwmChannel);
    // Initialize brightness
    currentBrightness = hornBrightness;
    targetBrightness = hornBrightness;
    pwmValue = fastMap<int>(currentBrightness, 0, 100, hornMinBrightness, hornMaxBrightness);
    ledcWrite(hornPwmChannel, pwmValue);
}

int HornLED::getBrightness() const {
    return currentBrightness;
}

void HornLED::setBrightness(int value, int speed) {
    targetBrightness = fastClamp<int>(value, 0, 100);  // Clamp to valid range (optimized)
    fadeSpeed = max(1, speed);                         // Ensure fade speed is at least 1
}

void HornLED::update() {
    unsigned long currentMillis = millis();

    // Only proceed if the time interval has elapsed
    if (currentMillis - previousMillis >= 20) {  // Faster update: 20ms instead of 100ms
        previousMillis = currentMillis;

        // Adjust brightness toward the target
        if (currentBrightness != targetBrightness) {
            int step = (currentBrightness < targetBrightness) ? fadeSpeed : -fadeSpeed;
            currentBrightness += step;

            // Clamp brightness to avoid overshooting
            if ((step > 0 && currentBrightness > targetBrightness) ||
                (step < 0 && currentBrightness < targetBrightness)) {
                currentBrightness = targetBrightness;
            }
        }

        // Map brightness to the target PWM value (optimized fastMap)
        float targetPwm = fastMap<float>(currentBrightness, 0, 100, hornMinBrightness, hornMaxBrightness);

        // Gradually adjust the actual PWM value
        if (abs(pwmValue - targetPwm) > 0.5) {  // Only update if there's a noticeable difference
            // More aggressive fade calculation for faster response
            pwmValue += (targetPwm - pwmValue) * fadeSpeed / 50.0f;

            // Snap to the target if close enough
            if (abs(pwmValue - targetPwm) < 1.0f) {
                pwmValue = targetPwm;
            }
            ledcWrite(hornPwmChannel, round(pwmValue));  // Write the new PWM value
        }
    }
}
