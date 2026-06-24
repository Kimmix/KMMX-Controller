#include "Fan.h"

#if HAS_FAN_CONTROL

#include "Utils/Utils.h"  // For fastMap, fastClamp utilities

constexpr uint8_t FAN_PWM_CHANNEL = 1;      // PWM channel (horn uses 0)
constexpr uint16_t FAN_PWM_FREQ = 25000;    // 25kHz (standard for PC fans)
constexpr uint8_t FAN_PWM_RESOLUTION = 8;   // 8-bit (0-255)

Fan::Fan() {
    // Initialize PWM for fan control
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM_PIN, FAN_PWM_CHANNEL);

    // Start with fan off for safety
    currentSpeed = 0;
    targetSpeed = 0;
    pwmValue = 0.0f;
    ledcWrite(FAN_PWM_CHANNEL, 0);

    Serial.println("Fan controller initialized (IO40=PWM)");
}

void Fan::setSpeed(int speed, int fadeSpeed) {
    targetSpeed = fastClamp<int>(speed, 0, 100);
    transitionSpeed = fastClamp<int>(fadeSpeed, 1, 100);
}

int Fan::getSpeed() const {
    return currentSpeed;
}

void Fan::update() {
    // Update speed transitions (similar to HornLED)
    if (currentSpeed != targetSpeed) {
        // Calculate step size based on transition speed
        int step = max(1, abs(targetSpeed - currentSpeed) * transitionSpeed / 50);

        if (currentSpeed < targetSpeed) {
            currentSpeed = min(currentSpeed + step, targetSpeed);
        } else {
            currentSpeed = max(currentSpeed - step, targetSpeed);
        }

        // Map speed (0-100) to PWM value (0-255)
        float targetPwm = fastMap<float>(currentSpeed, 0, 100, 0, 255);

        // Smooth PWM transition
        if (abs(pwmValue - targetPwm) > 0.5f) {
            pwmValue += (targetPwm - pwmValue) * 0.3f;  // Smooth interpolation

            if (abs(pwmValue - targetPwm) < 1.0f) {
                pwmValue = targetPwm;
            }

            ledcWrite(FAN_PWM_CHANNEL, round(pwmValue));
        }
    }
}

#endif // HAS_FAN_CONTROL
