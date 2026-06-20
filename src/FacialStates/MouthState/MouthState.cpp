#include "MouthState.h"
#include "Utils/Utils.h"
#include <Arduino.h>

MouthState::MouthState(Hub75DMA* display) : display(display) {
    // Initialize wah animation (60 frames, bouncy/elastic feel)
    TimeBasedAnimation::init(wahAnim, mouthWahAnimation, wahLength, TimeBasedAnimation::CONFIG_WAH);

    // Initialize idle breathing animation (60 frames, slow breathing)
    TimeBasedAnimation::init(idleAnim, defaultAnimation, defaultAnimationLength, TimeBasedAnimation::CONFIG_BREATHING);

    // Initialize angry animation (20 frames, quick transition)
    TimeBasedAnimation::init(angryAnim, mouthAngryAnimation, angryLength, TimeBasedAnimation::CONFIG_TRANSITION);

    // Initialize animations with transition + loop pattern
    // Eh: no loop (plays once)
    initAnimationData(ehData, mouthEhAnimation, ehLength, 0, TimeBasedAnimation::CONFIG_ANTICIPATION);

    // Pout: loop last 10 frames
    initAnimationData(poutData, mouthPoutAnimation, poutLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);

    // Drooling: loop last 20 frames
    initAnimationData(droolingData, mouthDroolingAnimation, droolingLength, 20, TimeBasedAnimation::CONFIG_BREATHING);

    // Angry: loop last 20 frames
    initAnimationData(angryNewData, mouthAngryNewAnimation, angryNewLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);

    // Lower: loop last 20 frames
    initAnimationData(lowerData, mouthLowerAnimation, lowerLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);

    // Shock: loop last 20 frames
    initAnimationData(shockData, mouthShockAnimation, shockLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);

    // Small: loop last 20 frames
    initAnimationData(smallData, mouthSmallAnimation, smallLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);

    // Worry: loop last 20 frames
    initAnimationData(worryData, mouthWorryAnimation, worryLength, 20, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);
}

void MouthState::startMic() {
    viseme.initMic();
    startVisemeTask();
}

void MouthState::update() {
    if (customResetDuration > 0) {
        if (millis() - stateStartTime >= customResetDuration) {
            customResetDuration = 0;
            setState(prevState, false, 0);
            return;
        }
    }

    switch (currentState) {
        case MouthStateEnum::IDLE:
            movingMouth();
            drawDefault();
            break;
        case MouthStateEnum::BOOP:
            display->drawMouth(mouthOH15);
            break;
        case MouthStateEnum::ANGRYBOOP:
            angryBoop();
            break;
        case MouthStateEnum::TALKING:
            if (visemeFrame == mouthDefault) {
                drawDefault();
            } else {
                display->drawMouth(visemeFrame);
            }
            break;
        case MouthStateEnum::WAH: {
            const uint8_t* wahFrame = TimeBasedAnimation::update(wahAnim);
            display->drawMouth(wahFrame);
            break;
        }
        case MouthStateEnum::EH:
            playAnimationWithLoop(ehData);
            break;
        case MouthStateEnum::POUT:
            playAnimationWithLoop(poutData);
            break;
        case MouthStateEnum::DROOLING:
            playAnimationWithLoop(droolingData);
            break;
        case MouthStateEnum::ANGRY:
            playAnimationWithLoop(angryNewData);
            break;
        case MouthStateEnum::LOWER:
            playAnimationWithLoop(lowerData);
            break;
        case MouthStateEnum::SHOCK:
            playAnimationWithLoop(shockData);
            break;
        case MouthStateEnum::SMALL:
            playAnimationWithLoop(smallData);
            break;
        case MouthStateEnum::WORRY:
            playAnimationWithLoop(worryData);
            break;
        default:
            break;
    }
}

void MouthState::setState(MouthStateEnum newState, bool isPersistent, unsigned long durationMs) {
    // Mark as transitioning if state is changing
    if (currentState != newState) {
        isTransitioning = true;
    }

    // State-specific initialization
    switch (newState) {
        case MouthStateEnum::ANGRYBOOP:
            TimeBasedAnimation::reset(angryAnim);
            break;
        case MouthStateEnum::EH:
            resetAnimation(ehData);
            break;
        case MouthStateEnum::POUT:
            resetAnimation(poutData);
            break;
        case MouthStateEnum::DROOLING:
            resetAnimation(droolingData);
            break;
        case MouthStateEnum::ANGRY:
            resetAnimation(angryNewData);
            break;
        case MouthStateEnum::LOWER:
            resetAnimation(lowerData);
            break;
        case MouthStateEnum::SHOCK:
            resetAnimation(shockData);
            break;
        case MouthStateEnum::SMALL:
            resetAnimation(smallData);
            break;
        case MouthStateEnum::WORRY:
            resetAnimation(worryData);
            break;
        default:
            break;
    }

    // Update state and timestamp
    currentState = newState;
    stateStartTime = millis();
    customResetDuration = durationMs;  // Store custom duration (0 = infinite)

    // Only save as previous state if persistent
    if (isPersistent) {
        savePrevState(currentState);
    }
}

void MouthState::savePrevState(MouthStateEnum newState) {
    prevState = newState;
}

void MouthState::playPrevState() {
    setState(prevState, false, 0);
}

MouthStateEnum MouthState::getState() const {
    return currentState;
}

void MouthState::setSensorData(const SensorData& data) {
    sensorData = data;
}

void MouthState::drawDefault() {
    display->drawMouth(mouthFrame);
}

void MouthState::resetMovingMouth() {
    TimeBasedAnimation::reset(idleAnim);
}

void MouthState::setSlowAnimation(bool slow) {
    if (slow) {
        TimeBasedAnimation::setConfig(idleAnim, TimeBasedAnimation::CONFIG_BREATHING_SLOW);
    } else {
        TimeBasedAnimation::setConfig(idleAnim, TimeBasedAnimation::CONFIG_BREATHING);
    }
}

void MouthState::movingMouth() {
    mouthFrame = TimeBasedAnimation::update(idleAnim);
}

void MouthState::angryBoop() {
    const uint8_t* currentFrame = TimeBasedAnimation::update(angryAnim);

    if (isTransitioning) {
        display->drawMouth(currentFrame);

        // Check if transition animation is complete
        if (TimeBasedAnimation::isComplete(angryAnim)) {
            isTransitioning = false;
        }
    } else {
        // Hold at frame near the end (angryLength - 8)
        display->drawMouth(mouthAngryAnimation[angryLength - 8]);
    }
}

void MouthState::startVisemeTask() {
    Serial.println(F("Start viseme task..."));
    xTaskCreatePinnedToCore(visemeRenderingTask, "VisemeTask", 2048, this, 2, &visemeTaskHandle, 0);
}

void MouthState::visemeRenderingTask(void* parameter) {
    MouthState* mouthState = reinterpret_cast<MouthState*>(parameter);
    bool wasTalking = false;

    while (true) {
        if (mouthState->getState() != MouthStateEnum::TALKING) {
            mouthState->visemeFrame = mouthDefault;
            wasTalking = false;
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (!wasTalking) {
            mouthState->viseme.reset();
            wasTalking = true;
        }

        mouthState->visemeFrame = mouthState->viseme.renderViseme();
    }
}

void MouthState::initAnimationData(MouthAnimationData& data, const uint8_t** frames, uint8_t frameCount,
                                   uint8_t loopFrameCount, const TimeBasedAnimConfig& loopConfig) {
    data.frames = frames;
    data.frameCount = frameCount;
    data.loopFrameCount = loopFrameCount;

    // Initialize transition animation (full animation, plays once)
    TimeBasedAnimation::init(data.transitionAnim, frames, frameCount, TimeBasedAnimation::CONFIG_TRANSITION);
    if (loopFrameCount > 0) {
        TimeBasedAnimation::init(data.loopAnim, &frames[frameCount - loopFrameCount], loopFrameCount, loopConfig);
    }
}

void MouthState::resetAnimation(MouthAnimationData& data) {
    TimeBasedAnimation::reset(data.transitionAnim);
    if (data.loopFrameCount > 0) {
        TimeBasedAnimation::reset(data.loopAnim);
    }
}

void MouthState::playAnimationWithLoop(MouthAnimationData& animData) {
    const uint8_t* currentFrame;

    if (isTransitioning) {
        // During transition: use full animation
        currentFrame = TimeBasedAnimation::update(animData.transitionAnim);

        // Also update loop animation in the background so it's in sync when we switch
        if (animData.loopFrameCount > 0) {
            TimeBasedAnimation::update(animData.loopAnim);
        }

        // Check if transition animation is complete
        if (TimeBasedAnimation::isComplete(animData.transitionAnim)) {
            isTransitioning = false;
            // Loop animation is already running in sync, no reset needed
        }
    } else {
        // After transition: loop only the last N frames (or hold on last frame if no loop)
        if (animData.loopFrameCount > 0) {
            currentFrame = TimeBasedAnimation::update(animData.loopAnim);
        } else {
            // No loop - hold on last frame
            currentFrame = animData.frames[animData.frameCount - 1];
        }
    }

    display->drawMouth(currentFrame);
}
