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
    static bool eyeBoopActive = false;  // Track if eye boop animation is active

    if (millis() < nextBoop) {
        return;
    }

    nextBoop = millis() + 50;
    BoopStatus status = boop.getBoop(sensorBuffer[activeBuffer].proximity);

    // Boop completed (reached boopMaxThreshold)
    if (status.isBoop) {
        handleBoopCompleted(status.boopSpeed, eyeBoopActive);
        wasBooped = true;
        wasInBoopRange = false;
        eyeBoopActive = true;  // Mark eye animation as active
    }
    // Object in boop range (boopMinThreshold < value < boopMaxThreshold)
    else if (status.isInRange) {
        handleBoopInRange();
        wasInBoopRange = true;
    }
    // Continuous boop (held at boopMaxThreshold)
    else if (status.isContinuous) {
        handleBoopContinuous(eyeBoopActive);
        wasBooped = true;
        wasInBoopRange = false;
    }
    // Angry state (value >= 1023, too close)
    else if (status.isAngry && !lastIsAngry) {
        handleBoopAngry();
        wasBooped = false;
        wasInBoopRange = false;
        eyeBoopActive = false;  // Reset eye animation tracking
    }
    // Idle (no boop conditions)
    else {
        handleBoopIdle(wasInBoopRange, wasBooped);
        wasInBoopRange = false;
        wasBooped = false;
        eyeBoopActive = false;  // Reset eye animation tracking
    }

    lastIsAngry = status.isAngry;
}

/**
 * Handle completed boop state (reached boopMaxThreshold).
 */
void KMMXController::handleBoopCompleted(float speed, bool eyeAlreadyActive) {
    fxState.setFlyingSpeed(speed);
    fxState.setState(FXStateEnum::Heart);

    // Only set eye state if not already active (prevents animation reset)
    if (!eyeAlreadyActive && eyeState.getState() != EyeStateEnum::BOOP) {
        eyeState.setState(EyeStateEnum::BOOP, false, 0);  // No timeout while in boop
    }
    setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 700);

    resetIdleTime();
    statusLED.setColor(Color::CYAN);
}

/**
 * Handle in-range boop state (object approaching).
 */
void KMMXController::handleBoopInRange() {
    setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 0);  // No timeout while in range

    if (isSleeping) {
        resetIdleTime();
    }
    statusLED.setColor(Color::LIGHT_PINK);
}

/**
 * Handle continuous boop state (held at boopMaxThreshold).
 */
void KMMXController::handleBoopContinuous(bool eyeAlreadyActive) {
    // Only set eye state if not already active (prevents animation reset during hold)
    if (!eyeAlreadyActive && eyeState.getState() != EyeStateEnum::BOOP) {
        eyeState.setState(EyeStateEnum::BOOP, false, 0);  // No timeout while holding
    }
    setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 0);  // No timeout while holding

    statusLED.setColor(Color::PINK);
}

/**
 * Handle angry boop state (too close, value >= 1023).
 */
void KMMXController::handleBoopAngry() {
    nextBoop = millis() + 1500;
    eyeState.setState(EyeStateEnum::ANGRY, false, 1500);
    mouthState.setState(MouthStateEnum::ANGRYBOOP, false, 1500);
    resetIdleTime();
    statusLED.setColor(Color::RED);
}

/**
 * Handle idle boop state (no active boop condition).
 */
void KMMXController::handleBoopIdle(bool wasInRange, bool wasBooped) {
    // If just left boop range or continuous boop, set timeout to fade out
    if ((wasInRange || wasBooped) && eyeState.getState() == EyeStateEnum::BOOP) {
        eyeState.setState(EyeStateEnum::BOOP, false, 2500);
    }
    if ((wasInRange || wasBooped) && mouthState.getState() == MouthStateEnum::BOOP) {
        mouthState.setState(MouthStateEnum::BOOP, false, 700);
    }

    // Show sad if approached but didn't complete boop
    if (wasInRange && !wasBooped) {
        setStateIfDifferent(eyeState, EyeStateEnum::SAD, 3000);
        statusLED.setColor(Color::BLUE);
    }
}

/**
 * Helper to set state only if it's different from current state.
 * Prevents unnecessary animation resets.
 */
template<typename StateType, typename EnumType>
void KMMXController::setStateIfDifferent(StateType& state, EnumType targetState, unsigned long timeout) {
    if (state.getState() != targetState) {
        state.setState(targetState, false, timeout);
    }
}
