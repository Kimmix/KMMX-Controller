#include "MouthState.h"
#include "Utils/Utils.h"
#include <Arduino.h>

MouthState::MouthState(Hub75DMA* display) : display(display) {
    // Initialize wah animation (60 frames, bouncy/elastic feel)
    TimeBasedAnimation::init(wahAnim, mouthWahAnimation, wahLength, TimeBasedAnimation::CONFIG_WAH);

    // Initialize idle breathing animation (60 frames, slow breathing)
    TimeBasedAnimation::init(idleAnim, defaultAnimation, defaultAnimationLength, TimeBasedAnimation::CONFIG_BREATHING);

    // Initialize angry animation (20 frames, quick transition)
    TimeBasedAnimation::init(angryAnim, mouthAngryAnimation, angryLength, TimeBasedAnimation::CONFIG_TRANSITION_END);

    // Initialize animations with transition + loop pattern
    initAnimationData(boopData, viseme.getFrames(Viseme::OH), 60, 20,
                      TimeBasedAnimation::CONFIG_SMOOTH_LOOP);
    boopData.loopAnim.config.durationMs = boopData.loopAnim.config.durationMs * boopLoopDurationPercent / 100;

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
    visemeFrame = mouthDefault1;
    if (viseme.initMic()) startVisemeTask();
}

void MouthState::update() {
    if (isReturning) {
        display->drawMouth(TimeBasedAnimation::update(returnAnim));
        if (TimeBasedAnimation::isComplete(returnAnim)) {
            isReturning = false;
            applyState(pendingState, pendingPersistent, pendingDuration);
        }
        return;
    }

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
            if (boopCompleted) {
                display->drawMouth(boopData.frames[boopData.frameCount - 1]);
            } else if (sensorData.proximity > boopMinThreshold && sensorData.proximity < boopMaxThreshold) {
                float progress = static_cast<float>(constrain(sensorData.proximity, boopMinThreshold, boopMaxThreshold) - boopMinThreshold) /
                                 (boopMaxThreshold - boopMinThreshold);
                progress = progress * progress * (3.0f - 2.0f * progress);
                short targetFrame = roundf(progress * (boopData.frameCount - 1));
                short currentFrame = boopData.transitionAnim.currentFrameIndex;
                currentFrame += constrain(targetFrame - currentFrame, -1, 1);
                TimeBasedAnimation::reset(boopData.transitionAnim, currentFrame);
                display->drawMouth(boopData.frames[currentFrame]);
            } else {
                playAnimationWithLoop(boopData);
            }
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
    if (isReturning) {
        if (newState == currentState) {
            isReturning = false;
        } else {
            pendingState = newState;
            pendingPersistent = isPersistent;
            pendingDuration = durationMs;
        }
        return;
    }

    const uint8_t** frames;
    uint8_t frameCount;
    if (currentState != newState && getAnimationFrames(currentState, frames, frameCount)) {
        TimeBasedAnimation::init(returnAnim, frames, frameCount, TimeBasedAnimation::CONFIG_TRANSITION_END);
        returnAnim.isReversing = true;
        isReturning = true;
        pendingState = newState;
        pendingPersistent = isPersistent;
        pendingDuration = durationMs;
        return;
    }

    applyState(newState, isPersistent, durationMs);
}

void MouthState::applyState(MouthStateEnum newState, bool isPersistent, unsigned long durationMs) {
    if (newState != MouthStateEnum::BOOP) boopCompleted = false;

    // Mark as transitioning if state is changing
    if (currentState != newState) {
        isTransitioning = true;
    }

    // State-specific initialization
    switch (newState) {
        case MouthStateEnum::BOOP:
            resetAnimation(boopData);
            break;
        case MouthStateEnum::ANGRYBOOP:
            TimeBasedAnimation::reset(angryAnim);
            break;
        case MouthStateEnum::WAH:
            TimeBasedAnimation::reset(wahAnim);
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
    prevState = (newState == MouthStateEnum::BOOP || newState == MouthStateEnum::ANGRYBOOP) ? MouthStateEnum::IDLE : newState;
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

void MouthState::setBoopCompleted(bool completed) {
    boopCompleted = completed;
    stateStartTime = millis();
    customResetDuration = 0;
}

void MouthState::releaseBoop(unsigned long durationMs) {
    boopCompleted = false;
    isTransitioning = false;
    TimeBasedAnimation::reset(boopData.loopAnim, boopData.loopFrameCount - 1);
    stateStartTime = millis();
    customResetDuration = durationMs;
}

void MouthState::drawDefault() {
    display->drawMouth(mouthFrame);
}

void MouthState::resetMovingMouth() {
    TimeBasedAnimation::reset(idleAnim);
}

void MouthState::setSlowAnimation(bool slow) {
    slowIdleAnimation = slow;
    if (slow) {
        TimeBasedAnimation::setConfig(idleAnim, TimeBasedAnimation::CONFIG_BREATHING_SLOW);
    } else {
        TimeBasedAnimation::setConfig(idleAnim, TimeBasedAnimation::CONFIG_BREATHING);
    }
    idleAnim.frameCount = defaultAnimationLength;
}

void MouthState::movingMouth() {
    const bool wasReversing = idleAnim.isReversing;
    mouthFrame = TimeBasedAnimation::update(idleAnim);

    if (!wasReversing && idleAnim.isReversing && idleSigh) {
        idleAnim.config.durationMs = idleAnim.config.durationMs * 6 / 5;
    } else if (wasReversing && !idleAnim.isReversing) {
        const TimeBasedAnimConfig& base = slowIdleAnimation
                                              ? TimeBasedAnimation::CONFIG_BREATHING_SLOW
                                              : TimeBasedAnimation::CONFIG_BREATHING;
        const bool recoveryBreath = idleSigh;
        idleSigh = !recoveryBreath && --breathsUntilSigh == 0;
        if (idleSigh) breathsUntilSigh = random(10, 21);

        idleAnim.frameCount = idleSigh ? defaultAnimationLength
                                      : recoveryBreath ? 52 : random(52, defaultAnimationLength + 1);
        const unsigned long depth = idleAnim.frameCount - 52;
        const unsigned long durationPercent = 90 + depth * 20 / (defaultAnimationLength - 52);
        const unsigned long pausePercent = 75 + depth * 50 / (defaultAnimationLength - 52);
        idleAnim.config.durationMs = base.durationMs * durationPercent / 100;
        idleAnim.config.pauseAtStartMs = base.pauseAtStartMs * pausePercent / 100;
        idleAnim.config.pauseAtEndMs = base.pauseAtEndMs * pausePercent / 100 * (idleSigh ? 2 : 1);
    }
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
    if (xTaskCreatePinnedToCore(visemeRenderingTask, "VisemeTask", 4096, this, 2, &visemeTaskHandle, 0) != pdPASS)
        viseme.taskFailed();
}

void MouthState::visemeRenderingTask(void* parameter) {
    auto* mouthState = static_cast<MouthState*>(parameter);
    while (true) {
        const bool talking = mouthState->getState() == MouthStateEnum::TALKING;
        mouthState->visemeFrame = mouthState->viseme.renderViseme(talking);
        if (mouthState->viseme.snapshot().status.mic == LipSync::MicState::ReadError)
            vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void MouthState::initAnimationData(MouthAnimationData& data, const uint8_t** frames, uint8_t frameCount,
                                   uint8_t loopFrameCount, const TimeBasedAnimConfig& loopConfig) {
    data.frames = frames;
    data.frameCount = frameCount;
    data.loopFrameCount = loopFrameCount;

    // Initialize transition animation (full animation, plays once)
    TimeBasedAnimation::init(data.transitionAnim, frames, frameCount, TimeBasedAnimation::CONFIG_TRANSITION_END);
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

bool MouthState::getAnimationFrames(MouthStateEnum state, const uint8_t**& frames, uint8_t& frameCount) {
    switch (state) {
        case MouthStateEnum::BOOP: frames = boopData.frames; frameCount = boopData.frameCount; return true;
        case MouthStateEnum::ANGRYBOOP: frames = mouthAngryAnimation; frameCount = angryLength; return true;
        case MouthStateEnum::WAH: frames = mouthWahAnimation; frameCount = wahLength; return true;
        case MouthStateEnum::EH: frames = ehData.frames; frameCount = ehData.frameCount; return true;
        case MouthStateEnum::POUT: frames = poutData.frames; frameCount = poutData.frameCount; return true;
        case MouthStateEnum::DROOLING: frames = droolingData.frames; frameCount = droolingData.frameCount; return true;
        case MouthStateEnum::ANGRY: frames = angryNewData.frames; frameCount = angryNewData.frameCount; return true;
        case MouthStateEnum::LOWER: frames = lowerData.frames; frameCount = lowerData.frameCount; return true;
        case MouthStateEnum::SHOCK: frames = shockData.frames; frameCount = shockData.frameCount; return true;
        case MouthStateEnum::SMALL: frames = smallData.frames; frameCount = smallData.frameCount; return true;
        case MouthStateEnum::WORRY: frames = worryData.frames; frameCount = worryData.frameCount; return true;
        default: return false;
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
