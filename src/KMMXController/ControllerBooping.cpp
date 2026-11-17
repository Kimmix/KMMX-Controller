#include "KMMXController.h"
#include "MotionDetectionConfig.h"

void KMMXController::handleBoop() {
    const unsigned long now = millis();
    if (now - lastBoopUpdate < 50) return;
    lastBoopUpdate = now;

    const BoopResult result = boop.update(currentSensor.proximity, now);

    switch (result.event) {
        case BoopEvent::APPROACHING:
            eyeState.setBoopCompleted(false);
            mouthState.setBoopCompleted(false);
            setStateIfDifferent(eyeState, EyeStateEnum::BOOP, 0);
            setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 0);
            if (isSleeping) resetIdleTime();
            break;

        case BoopEvent::COMPLETED:
            fxState.setFlyingSpeed(result.speed);
            fxState.setState(FXStateEnum::Heart);
            setStateIfDifferent(eyeState, EyeStateEnum::BOOP, 0);
            setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 0);
            eyeState.setBoopCompleted(true);
            mouthState.setBoopCompleted(true);
            resetIdleTime();
            break;

        case BoopEvent::HELD:
            setStateIfDifferent(eyeState, EyeStateEnum::BOOP, 0);
            setStateIfDifferent(mouthState, MouthStateEnum::BOOP, 0);
            break;

        case BoopEvent::INCOMPLETE_RELEASE:
            if (mouthState.getState() == MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 700);
            }
            if (eyeState.getState() == EyeStateEnum::BOOP) {
                eyeState.setState(EyeStateEnum::BOOP, false, 700);  // sad face removed on this branch
            }
            break;

        case BoopEvent::RELEASED:
            if (eyeState.getState() == EyeStateEnum::BOOP) {
                eyeState.releaseBoop(boopFaceDuration);
            }
            if (mouthState.getState() == MouthStateEnum::BOOP) {
                mouthState.releaseBoop(boopFaceDuration);
            }
            break;

        case BoopEvent::IDLE:
            break;
    }
}

template<typename StateType, typename EnumType>
void KMMXController::setStateIfDifferent(StateType& state, EnumType targetState, unsigned long timeout) {
    if (state.getState() != targetState) state.setState(targetState, false, timeout);
}
