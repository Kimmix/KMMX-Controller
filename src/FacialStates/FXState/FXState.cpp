#include "FXState.h"
#include "FacialStates/EyeState/EyeState.h"
#include <Arduino.h>

FXState::FXState(Hub75DMA* displayPtr, EyeState* eyeStatePtr)
    : display(displayPtr), eyeState(eyeStatePtr), flyingHeart(displayPtr), weepingTears(displayPtr) {}

void FXState::update() {
    handleAutoStateChanges();

    switch (currentState) {
        case FXStateEnum::IDLE:
            break;
        case FXStateEnum::Heart:
            flyingHeartState();
            blushingState();
            break;
        case FXStateEnum::Blush:
            blushingState();
            break;
        case FXStateEnum::Tears:
            weepingTearsState();
            break;
        default:
            currentState = FXStateEnum::IDLE;
            break;
    }
}

void FXState::handleAutoStateChanges() {
    if (eyeState && eyeState->getState() == EyeStateEnum::SMILE) {
        // currentState = FXStateEnum::Blush;
    } else if (eyeState && eyeState->getState() == EyeStateEnum::CRY) {
        // Keep tears active while in CRY state
        if (currentState != FXStateEnum::Tears) {
            currentState = FXStateEnum::Tears;
            weepingTears.reset();  // Restart streams when entering CRY
        }
    } else {
        // Eye is not in SMILE or CRY, reset FX effects
        if (currentState == FXStateEnum::Blush) {
            currentState = FXStateEnum::IDLE;
        } else if (currentState == FXStateEnum::Tears) {
            currentState = FXStateEnum::IDLE;
            weepingTears.resetAll();  // Clean up tears
        }
    }
}

void FXState::setState(FXStateEnum newState) {
    if (newState == FXStateEnum::Heart) {
        resetHeart = millis();
        flyingHeart.reset();
    } else if (newState == FXStateEnum::Tears) {
        resetTears = millis();
        weepingTears.reset();
    }
    currentState = newState;
}

FXStateEnum FXState::getState() const {
    return currentState;
}

void FXState::flyingHeartState() {
    flyingHeart.renderHeart();
    if (millis() - resetHeart >= 5000) {
        currentState = FXStateEnum::IDLE;
        flyingHeart.resetAll();
    }
}

void FXState::blushingState() {
    const int blushOffsetX = 27;
    const int blushOffsetY = 12;
    display->drawBitmap(blushingFX, 14, 5, blushOffsetX, blushOffsetY, 255, 0, 0);
    display->drawBitmap(blushingFX, 14, 5, panelResX + (panelResX - blushOffsetX - 14), blushOffsetY, 255, 0, 0);
}

void FXState::weepingTearsState() {
    weepingTears.renderTears();
    // Optional: auto-reset after duration (currently runs indefinitely)
    // if (millis() - resetTears >= 10000) {
    //     currentState = FXStateEnum::IDLE;
    //     weepingTears.resetAll();
    // }
}

void FXState::setFlyingSpeed(float i) {
    flyingHeart.setSpeed(i);
}

void FXState::setTearIntensity(float i) {
    weepingTears.setIntensity(i);
}

void FXState::setEyeStatePtr(EyeState* eyeStatePtr) {
    eyeState = eyeStatePtr;
}
