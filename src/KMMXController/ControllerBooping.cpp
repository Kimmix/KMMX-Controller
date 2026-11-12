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

        case BoopEvent::TOO_CLOSE:
            fxState.setState(FXStateEnum::IDLE);
            setStateIfDifferent(eyeState, EyeStateEnum::ANGRY, 0);
            setStateIfDifferent(mouthState, MouthStateEnum::ANGRYBOOP, 0);
            resetIdleTime();
            break;

        case BoopEvent::LOCKED_OUT:
            boopLockedOut = true;  // cleared in the render loop once proximity drops below boopMinThreshold
            if (eyeState.getState() == EyeStateEnum::ANGRY) {
                eyeState.setState(EyeStateEnum::ANGRY, false, 1500);
            }
            if (mouthState.getState() == MouthStateEnum::ANGRYBOOP) {
                mouthState.setState(MouthStateEnum::ANGRYBOOP, false, 1500);
            }
            break;

        case BoopEvent::INCOMPLETE_RELEASE:
            if (mouthState.getState() == MouthStateEnum::BOOP) {
                mouthState.setState(MouthStateEnum::BOOP, false, 700);
            }
            setStateIfDifferent(eyeState, EyeStateEnum::SAD, 3000);
            break;

        case BoopEvent::RELEASED:
            if (eyeState.getState() == EyeStateEnum::BOOP) {
                eyeState.releaseBoop(boopFaceDuration);
            } else if (eyeState.getState() == EyeStateEnum::ANGRY) {
                eyeState.setState(EyeStateEnum::ANGRY, false, 1500);
            }
            if (mouthState.getState() == MouthStateEnum::BOOP) {
                mouthState.releaseBoop(boopFaceDuration);
            } else if (mouthState.getState() == MouthStateEnum::ANGRYBOOP) {
                mouthState.setState(MouthStateEnum::ANGRYBOOP, false, 1500);
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
