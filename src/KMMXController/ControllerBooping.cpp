#include "KMMXController.h"

/**
 * Handle boop detection and state transitions.
 *
 * Processes normalized proximity data (0-1023) from VL6180X or APDS9930 sensor
 * through the Boop state machine. Updates every 50ms.
 */
void KMMXController::handleBoop() {
    static bool lastIsAngry = false;
    static bool wasInBoopRange = false;
    static bool wasBooped = false;

    if (millis() >= nextBoop) {
        nextBoop = millis() + 50;
        boop.getBoop(sensorBuffer[activeBuffer].proximity, inBoopRange, isBooping, boopSpeed, isContinuousBoop, isAngry);

        // Boop completed (reached boopMaxThreshold)
        if (isBooping) {
            fxState.setFlyingSpeed(boopSpeed);
            fxState.setState(FXStateEnum::Heart);
            // Prevent animation reset if already in BOOP state
            if (eyeState.getState() != EyeStateEnum::BOOP) {
                eyeState.setState(EyeStateEnum::BOOP, false, 2500);
            }
            if (mouthState.getState() != MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 700);
            }
            resetIdleTime();
            statusLED.setColor(Color::CYAN);
            wasBooped = true;
            wasInBoopRange = false;
        }
        // Object in boop range (boopMinThreshold < value < boopMaxThreshold)
        else if (inBoopRange) {
            if (mouthState.getState() != MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 0);  // No timeout while in range
            }
            if (isSleeping) {
                resetIdleTime();
            }
            statusLED.setColor(Color::LIGHT_PINK);
            wasInBoopRange = true;
        }
        // Continuous boop (held at boopMaxThreshold)
        else if (isContinuousBoop) {
            if (eyeState.getState() != EyeStateEnum::BOOP) {
                eyeState.setState(EyeStateEnum::BOOP, false, 2500);
            }
            if (mouthState.getState() != MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 0);  // No timeout while holding
            }
            statusLED.setColor(Color::PINK);
            wasBooped = true;
            wasInBoopRange = false;
        }
        // Angry state (value >= 1023, too close)
        else if (isAngry && !lastIsAngry) {
            nextBoop = millis() + 1500;
            eyeState.setState(EyeStateEnum::ANGRY, false, 1500);
            mouthState.setState(MouthStateEnum::ANGRYBOOP, false, 1500);
            resetIdleTime();
            statusLED.setColor(Color::RED);
            wasBooped = false;
            wasInBoopRange = false;
        }
        // Idle (no boop conditions)
        else {
            // If just left boop range or continuous boop, set timeout to fade out mouth
            if ((wasInBoopRange || wasBooped) && mouthState.getState() == MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 700);
            }

            // Show sad if approached but didn't complete boop
            if (wasInBoopRange && !wasBooped) {
                if (eyeState.getState() != EyeStateEnum::SAD) {
                    eyeState.setState(EyeStateEnum::SAD, false, 3000);
                    statusLED.setColor(Color::BLUE);
                }
            }
            wasInBoopRange = false;
            wasBooped = false;
        }
        lastIsAngry = isAngry;
    }
}
