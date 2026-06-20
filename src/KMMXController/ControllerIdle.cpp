#include "KMMXController.h"
#include "MotionDetectionConfig.h"
#include <cmath>

void KMMXController::resetIdleTime(KMMXController* controller) {
    controller->eyeState.playPrevState();
    controller->mouthState.playPrevState();  // Restore previous state, not just reset animation
    controller->mouthState.setSlowAnimation(false);
    controller->stillTime = 0;
    controller->motionCounter = 0;
    controller->isSleeping = false;
    controller->hornLED.setBrightness(controller->prevHornBright, 5);

    if (enableMotionDebug) {
        Serial.println("[IDLE] Reset - Motion detected, returning to active state");
    }
}

void KMMXController::resetIdleTime() {
    if (isSleeping) {
        eyeState.playPrevState();
        mouthState.playPrevState();  // Restore previous state, not just reset animation
        mouthState.setSlowAnimation(false);
    }
    stillTime = 0;
    motionCounter = 0;
    isSleeping = false;
    hornLED.setBrightness(prevHornBright, 5);

    if (enableMotionDebug) {
        Serial.println("[IDLE] Reset - Motion detected, returning to active state");
    }
}

void KMMXController::enterSleep(KMMXController* controller) {
    controller->prevHornBright = controller->hornLED.getBrightness();
    controller->hornLED.setBrightness(5);
    controller->eyeState.setState(EyeStateEnum::SLEEP, false, 0);  // Temporary, no timeout (manual control)
    controller->mouthState.setState(MouthStateEnum::IDLE, false, 0);  // Set mouth to idle
    controller->mouthState.setSlowAnimation(true);
    controller->isSleeping = true;

    if (enableMotionDebug) {
        Serial.println("[IDLE] Entering sleep mode");
    }
}

void KMMXController::checkIdleAndSleep(KMMXController* controller, unsigned long currentTime) {
    // Only check idle/sleep when in IDLE or SLEEP eye states
    if (controller->eyeState.getState() != EyeStateEnum::IDLE &&
        controller->eyeState.getState() != EyeStateEnum::SLEEP) {
        return;
    }

    const SensorData& current = controller->sensorBuffer[controller->activeBuffer];

    // Initialize baseline on first run
    if (!controller->baselineInitialized) {
        controller->baselineAccel = current;
        controller->baselineInitialized = true;
        controller->stillTime = currentTime;
        return;
    }

    // Calculate magnitude difference from baseline
    float magnitudeDelta = fabsf(current.accelMagnitude - controller->baselineAccel.accelMagnitude);

    // Select threshold based on current state
    float currentThreshold = controller->isSleeping ? sleepingAccThreshold : idleAccThreshold;

    // Check for motion
    bool motionDetected = (magnitudeDelta > currentThreshold);

    if (motionDetected) {
        // Motion detected - increment hysteresis counter
        controller->motionCounter++;
        // If sleeping, require sustained motion to wake up (hysteresis)
        if (controller->isSleeping) {
            if (controller->motionCounter >= motionHysteresisCount) {
                resetIdleTime(controller);
                // Don't update baseline here - let it update when still in new position
            }
            // Don't reset counter while sleeping - let it accumulate
        }
        // If not sleeping, motion just resets the idle timer (prevents sleep)
        else {
            controller->stillTime = currentTime;  // Reset idle timer
        }
    } else {
        // No motion detected
        // While sleeping, decay counter slowly instead of instant reset
        // This allows motion to accumulate even with gaps between movements
        if (controller->isSleeping) {
            // Decay by 1 every N samples (configurable decay rate)
            static uint8_t decayCounter = 0;
            decayCounter++;
            if (decayCounter >= motionCounterDecayRate && controller->motionCounter > 0) {
                controller->motionCounter--;
                decayCounter = 0;
            }
        } else {
            // When awake, instant reset
            controller->motionCounter = 0;
        }

        // Check if should enter sleep
        if (controller->stillTime == 0) {
            controller->stillTime = currentTime;
        } else if (!controller->isSleeping &&
                   (currentTime - controller->stillTime >= idleTimeout)) {
            // Been still long enough - go to sleep
            enterSleep(controller);
        }
    }
}
