#include "Fan.h"

#if HAS_FAN_CONTROL

#include "Utils/Utils.h"  // For fastMap, fastClamp utilities

constexpr uint8_t FAN_PWM_CHANNEL = 1;      // PWM channel (horn uses 0)
constexpr uint16_t FAN_PWM_FREQ = 25000;    // 25kHz (standard for PC fans)
constexpr uint8_t FAN_PWM_RESOLUTION = 8;   // 8-bit (0-255)

// Static instance pointer for ISR
Fan* Fan::instance = nullptr;

Fan::Fan() {
    // Initialize PWM for fan control
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM_PIN, FAN_PWM_CHANNEL);

    // Initialize tachometer pin
    pinMode(FAN_SPD_PIN, INPUT_PULLUP);

    // Attach interrupt for tachometer pulse counting
    instance = this;
    attachInterrupt(digitalPinToInterrupt(FAN_SPD_PIN), tachometerISR, FALLING);

    // Start with fan off for safety
    currentSpeed = 0;
    targetSpeed = 0;
    pwmValue = 0.0f;
    enabled = true;  // Enabled by default, but speed is 0
    ledcWrite(FAN_PWM_CHANNEL, 0);

    Serial.println("Fan controller initialized (IO40=PWM, IO41=TACH)");
}

void Fan::setSpeed(int speed, int fadeSpeed) {
    if (!enabled) {
        // If disabled, ignore speed commands
        targetSpeed = 0;
        return;
    }

    targetSpeed = fastClamp<int>(speed, 0, 100);
    transitionSpeed = fastClamp<int>(fadeSpeed, 1, 100);
}

int Fan::getSpeed() const {
    return currentSpeed;
}

void Fan::setEnabled(bool enable) {
    enabled = enable;
    if (!enabled) {
        targetSpeed = 0;  // Force fan off when disabled
    }
}

bool Fan::isEnabled() const {
    return enabled;
}

uint16_t Fan::getRPM() const {
    return currentRPM;
}

bool Fan::isConnected() const {
    return fanConnected;
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

    // Update RPM calculation periodically
    calculateRPM();
}

void Fan::calculateRPM() {
    uint32_t currentTime = millis();

    // Update RPM every second
    if (currentTime - lastRpmUpdate >= RPM_UPDATE_INTERVAL) {
        // Read pulse count atomically (disable interrupts briefly)
        noInterrupts();
        uint32_t pulses = pulseCount;
        pulseCount = 0;  // Reset counter
        interrupts();

        // Calculate RPM
        // Most 4-pin fans generate 2 pulses per revolution
        // RPM = (pulses / 2) * (60 seconds / measurement interval in seconds)
        // For 1 second interval: RPM = (pulses / 2) * 60 = pulses * 30
        currentRPM = pulses * 30;

        // Apply smoothing filter (simple moving average with previous value)
        if (lastPulseCount > 0) {
            currentRPM = (currentRPM + (lastPulseCount * 30)) / 2;
        }

        lastPulseCount = pulses;
        lastRpmUpdate = currentTime;
    }

    // Check fan connection status periodically
    if (currentTime - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
        // Fan is considered connected if:
        // 1. Fan speed is above 0, AND
        // 2. We're getting RPM readings above threshold
        // OR fan speed is 0 (can't detect when off, assume connected)
        if (currentSpeed == 0) {
            // Can't detect connection when fan is off, assume connected
            fanConnected = true;
        } else {
            // Fan is running - check if we're getting tachometer pulses
            fanConnected = (currentRPM >= MIN_RPM_THRESHOLD);

            // Log connection status changes
            static bool lastConnectedState = true;
            if (fanConnected != lastConnectedState) {
                if (fanConnected) {
                    Serial.println("[Fan] Fan connected and responding");
                } else {
                    Serial.println("[Fan] WARNING: Fan not responding (check connection)");
                }
                lastConnectedState = fanConnected;
            }
        }

        lastConnectionCheck = currentTime;
    }
}

// Interrupt Service Routine for tachometer
void IRAM_ATTR Fan::tachometerISR() {
    if (instance) {
        instance->pulseCount++;
    }
}

#endif // HAS_FAN_CONTROL
