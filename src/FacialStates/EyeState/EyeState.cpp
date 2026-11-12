#include "EyeState.h"
#include "Utils/Utils.h"
#include <Arduino.h>
#include "Bitmaps/Bitmaps.h"

EyeState::EyeState(Hub75DMA* display) : display(display), startSleepTime(millis()) {
    // Initialize blink animation (special case - no loop)
    TimeBasedAnimation::init(blinkAnim, blinkAnimation, blinkAnimationLength, TimeBasedAnimation::CONFIG_BLINK);

    // Initialize all animations with transition + loop pattern
    initAnimationData(boopData, boopAnimation, 48, 18, 2500, TimeBasedAnimation::CONFIG_BOUNCE_OVERSHOOT);        // Auto-reset after 2.5s
    boopData.loopAnim.config.durationMs = boopData.loopAnim.config.durationMs * boopLoopDurationPercent / 100;
    initAnimationData(arrowData, boopAnimation, 48, 18, 0, TimeBasedAnimation::CONFIG_BOUNCE_OVERSHOOT);          // No auto-reset (duplicate of boop for BLE)
    initAnimationData(oFaceData, oFaceAnimation, 48, 10, 0, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);              // No auto-reset
    initAnimationData(smileData, smileAnimation, 48, 10, 0, TimeBasedAnimation::CONFIG_SMILE_LOOP);               // No auto-reset, special ping-pong loop
    initAnimationData(angryData, eyeAngryAnimation, 48, 18, 0, TimeBasedAnimation::CONFIG_ANTICIPATION);          // No auto-reset
    initAnimationData(sadData, sadAnimation, 20, 8, 3000, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);                // Auto-reset after 3s
    initAnimationData(cryData, cryAnimation, 48, 12, 0, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);                  // No auto-reset, for photo shoots
    initAnimationData(doubtedData, doubtedAnimation, 48, 10, 0, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);          // No auto-reset, for photo shoots
    initAnimationData(roundedData, roundedAnimation, 48, 15, 0, TimeBasedAnimation::CONFIG_BOUNCE_OVERSHOOT);     // No auto-reset, for photo shoots
    initAnimationData(sharpData, sharpAnimation, 48, 18, 0, TimeBasedAnimation::CONFIG_ANTICIPATION);             // No auto-reset, focused state
    initAnimationData(giggleData, giggleAnimation, 48, 12, 0, TimeBasedAnimation::CONFIG_SMILE_LOOP);             // No auto-reset, playful state
    initAnimationData(unimpressedData, unimpressedAnimation, 48, 14, 0, TimeBasedAnimation::CONFIG_SMOOTH_LOOP);  // No auto-reset, for photo shoots

    // Initialize idle timers
    nextIdleAction = millis() + 1000;  // First idle action after 1s
    nextBlink = millis() + 3000;       // First blink after 3s
}

void EyeState::initAnimationData(AnimationData& data, const uint8_t** frames, uint8_t frameCount,
                                 uint8_t loopFrameCount, unsigned long autoResetDuration,
                                 const TimeBasedAnimConfig& loopConfig) {
    data.frames = frames;
    data.frameCount = frameCount;
    data.loopFrameCount = loopFrameCount;
    data.autoResetDuration = autoResetDuration;

    // Initialize transition animation (full animation, plays once)
    TimeBasedAnimation::init(data.transitionAnim, frames, frameCount, TimeBasedAnimation::CONFIG_TRANSITION_END);

    // Initialize loop animation (last N frames only)
    // Uses pointer arithmetic to reference the last N frames without duplicating data
    TimeBasedAnimation::init(data.loopAnim, &frames[frameCount - loopFrameCount], loopFrameCount, loopConfig);
}

void EyeState::resetAnimation(AnimationData& data) {
    TimeBasedAnimation::reset(data.transitionAnim);
    TimeBasedAnimation::reset(data.loopAnim);
}

AnimationData* EyeState::getAnimationData(EyeStateEnum state) {
    switch (state) {
        case EyeStateEnum::BOOP:
            return &boopData;
        case EyeStateEnum::ARROW:
            return &arrowData;
        case EyeStateEnum::OEYE:
            return &oFaceData;
        case EyeStateEnum::SMILE:
            return &smileData;
        case EyeStateEnum::ANGRY:
            return &angryData;
        case EyeStateEnum::SAD:
            return &sadData;
        case EyeStateEnum::CRY:
            return &cryData;
        case EyeStateEnum::DOUBTED:
            return &doubtedData;
        case EyeStateEnum::ROUNDED:
            return &roundedData;
        case EyeStateEnum::SHARP:
            return &sharpData;
        case EyeStateEnum::GIGGLE:
            return &giggleData;
        case EyeStateEnum::UNIMPRESSED:
            return &unimpressedData;
        default:
            return nullptr;
    }
}

void EyeState::update() {
    if (isReturning) {
        display->drawEye(TimeBasedAnimation::update(returnAnim));
        if (TimeBasedAnimation::isComplete(returnAnim)) {
            isReturning = false;
            applyState(pendingState, pendingPersistent, pendingDuration);
        }
        return;
    }

    // Check for custom duration auto-reset (PRIORITY 1)
    if (customResetDuration > 0) {
        if (millis() - stateStartTime >= customResetDuration) {
            customResetDuration = 0;  // Reset for next state
            setState(prevState, false, 0);  // Properly restore state (not just direct assignment)
            return;  // Exit early - state just changed
        }
    }

    // Check for animation-specific auto-reset (PRIORITY 2 - fallback)
    AnimationData* animData = getAnimationData(currentState);
    if (customResetDuration == 0 && animData && animData->autoResetDuration > 0 && !boopCompleted) {
        if (millis() - stateStartTime >= animData->autoResetDuration) {
            setState(prevState, false, 0);  // Properly restore state (not just direct assignment)
            return;  // Exit early - state just changed
        }
    }

    switch (currentState) {
        case EyeStateEnum::IDLE:
            idleFace();
            break;
        case EyeStateEnum::BLINK:
            blink();
            break;
        case EyeStateEnum::BOOP:
            if (boopCompleted) {
                display->drawEye(boopAnimation[arrayLength(boopAnimation) - 1]);
            } else if (sensorData.proximity > boopMinThreshold && sensorData.proximity < boopMaxThreshold) {
                float progress = static_cast<float>(constrain(sensorData.proximity, boopMinThreshold, boopMaxThreshold) - boopMinThreshold) /
                                 (boopMaxThreshold - boopMinThreshold);
                progress = progress * progress * (3.0f - 2.0f * progress);
                short targetFrame = roundf(progress * (arrayLength(boopAnimation) - 1));
                short currentFrame = boopData.transitionAnim.currentFrameIndex;
                currentFrame += constrain(targetFrame - currentFrame, -1, 1);
                TimeBasedAnimation::reset(boopData.transitionAnim, currentFrame);
                display->drawEye(boopAnimation[currentFrame]);
            } else {
                playAnimationWithLoop(boopData);
            }
            break;
        case EyeStateEnum::ARROW:
            playAnimationWithLoop(arrowData);
            break;
        case EyeStateEnum::GOOGLY:
            renderGooglyEye();
            break;
        case EyeStateEnum::OEYE:
            playAnimationWithLoop(oFaceData);
            break;
        case EyeStateEnum::HEART:
            display->drawEye(eyeHeart);
            break;
        case EyeStateEnum::SMILE:
            playAnimationWithLoop(smileData);
            break;
        case EyeStateEnum::ANGRY:
            playAnimationWithLoop(angryData);
            break;
        case EyeStateEnum::SLEEP:
            sleepFace();
            break;
        case EyeStateEnum::SAD:
            playAnimationWithLoop(sadData);
            break;
        case EyeStateEnum::CRY:
            playAnimationWithLoop(cryData);
            break;
        case EyeStateEnum::DOUBTED:
            playAnimationWithLoop(doubtedData);
            break;
        case EyeStateEnum::ROUNDED:
            playAnimationWithLoop(roundedData);
            break;
        case EyeStateEnum::SHARP:
            playAnimationWithLoop(sharpData);
            break;
        case EyeStateEnum::GIGGLE:
            playAnimationWithLoop(giggleData);
            break;
        case EyeStateEnum::UNIMPRESSED:
            playAnimationWithLoop(unimpressedData);
            break;
        default:
            break;
    }
}

void EyeState::setState(EyeStateEnum newState, bool isPersistent, unsigned long durationMs) {
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

    AnimationData* currentAnimation = getAnimationData(currentState);
    if (currentState != newState && currentAnimation) {
        TimeBasedAnimation::init(returnAnim, currentAnimation->frames, currentAnimation->frameCount,
                                 TimeBasedAnimation::CONFIG_TRANSITION_END);
        returnAnim.isReversing = true;
        isReturning = true;
        pendingState = newState;
        pendingPersistent = isPersistent;
        pendingDuration = durationMs;
        return;
    }

    applyState(newState, isPersistent, durationMs);
}

void EyeState::applyState(EyeStateEnum newState, bool isPersistent, unsigned long durationMs) {
    if (newState != EyeStateEnum::BOOP) boopCompleted = false;

    // Mark as transitioning if state is changing
    if (currentState != newState) {
        isTransitioning = true;
    }

    // State-specific initialization
    switch (newState) {
        case EyeStateEnum::BLINK:
            TimeBasedAnimation::reset(blinkAnim);
            break;

        case EyeStateEnum::SLEEP:
            resetSleepFace();
            break;

        case EyeStateEnum::SMILE:
            // Smile uses special transition config
            TimeBasedAnimation::setConfig(smileData.transitionAnim, TimeBasedAnimation::CONFIG_SMILE_TRANSITION);
            resetAnimation(smileData);
            break;

        case EyeStateEnum::BOOP:
        case EyeStateEnum::ARROW:
        case EyeStateEnum::OEYE:
        case EyeStateEnum::ANGRY:
        case EyeStateEnum::SAD:
        case EyeStateEnum::CRY:
        case EyeStateEnum::DOUBTED:
        case EyeStateEnum::ROUNDED:
        case EyeStateEnum::SHARP:
        case EyeStateEnum::GIGGLE:
        case EyeStateEnum::UNIMPRESSED: {
            // Reset animation data for states with transition + loop
            AnimationData* animData = getAnimationData(newState);
            if (animData) {
                resetAnimation(*animData);
            }
            break;
        }

        default:
            break;
    }

    // Update state and timestamp
    currentState = newState;
    stateStartTime = millis();
    customResetDuration = durationMs;  // Store custom duration (0 = infinite/use animation default)

    // Only save as previous state if persistent
    if (isPersistent) {
        savePrevState(currentState);
    }
}

void EyeState::savePrevState(EyeStateEnum newState) {
    prevState = newState == EyeStateEnum::BOOP ? EyeStateEnum::IDLE : newState;
}

void EyeState::playPrevState() {
    setState(prevState, false, 0);  // Restore previous state (temporary, manual control)
    resetSleepFace();
}

EyeStateEnum EyeState::getState() const {
    return currentState;
}

void EyeState::setSensorData(const SensorData& data) {
    sensorData = data;
}

void EyeState::setBoopCompleted(bool completed) {
    boopCompleted = completed;
    stateStartTime = millis();
    customResetDuration = 0;
}

void EyeState::releaseBoop(unsigned long durationMs) {
    boopCompleted = false;
    isTransitioning = false;
    TimeBasedAnimation::reset(boopData.loopAnim, boopData.loopFrameCount - 1);
    stateStartTime = millis();
    customResetDuration = durationMs;
}

void EyeState::idleFace() {
    display->drawEye(currentIdleBitmap);
    updateIdleMicroMovements();
    checkAndTriggerBlink();
}

void EyeState::updateIdleMicroMovements() {
    const unsigned long now = millis();
    if (now < nextIdleAction) return;

    if (pendingIdleFrame) {
        if (idleTransitionStep < 3) {
            currentIdleBitmap = idleTransitionFrames[pendingIdleFrame][idleTransitionStep++];
            nextIdleAction = now + 60;
            return;
        }
        currentIdleFrame = pendingIdleFrame;
        currentIdleBitmap = idleLookFrames[currentIdleFrame];
        pendingIdleFrame = 0;
        idleTransitionStep = 0;
        idleCorrectionMade = false;
        nextIdleAction = now + 600 + (esp_random() % 1200);
        return;
    }

    if (currentIdleFrame) {
        const uint8_t correctionFrame = idleCorrectionFrames[currentIdleFrame];
        if (!idleCorrectionMade && correctionFrame && (esp_random() % 100) < 65) {
            currentIdleFrame = correctionFrame;
            currentIdleBitmap = idleLookFrames[currentIdleFrame];
            idleCorrectionMade = true;
            nextIdleAction = now + 250 + (esp_random() % 550);
            return;
        }

        currentIdleFrame = 0;
        currentIdleBitmap = eyeDefault;
        idleCorrectionMade = false;
        nextIdleAction = now + 5000 + (esp_random() % 7000);
        nextBlink = now;
        return;
    }

    const uint32_t expression = esp_random() % 100;
    pendingIdleFrame = expression < 80 ? 1 + (esp_random() % 4)
                       : expression < 95 ? ((esp_random() % 2) ? 5 : 9)
                                         : 6 + (esp_random() % 3);
    nextIdleAction = now + 40;
}

void EyeState::checkAndTriggerBlink() {
    if (millis() < nextBlink) return;

    // Schedule next blink with varied interval
    nextBlink = millis() + selectBlinkInterval();

    // Decide if double blink (10% chance)
    shouldDoubleBlink = (esp_random() % 100) < 10;
    blinkCount = 0;
    blinkResumeAt = 0;

    // Select blink speed and initialize animation
    const TimeBasedAnimConfig* blinkConfig = selectBlinkSpeed(shouldDoubleBlink);
    TimeBasedAnimation::init(blinkAnim, blinkAnimation, blinkAnimationLength, *blinkConfig);

    currentState = EyeStateEnum::BLINK;
}

unsigned long EyeState::selectBlinkInterval() {
    uint32_t variance = esp_random() % 100;

    if (variance < 10) {
        // 10% - Quick interval (1-3s) - alert/active
        return 1000 + (esp_random() % 2000);
    } else if (variance < 80) {
        // 70% - Normal interval (5-5.7s)
        return 5000 + (esp_random() % 2500);
    } else {
        // 20% - Long interval (10-20s) - relaxed/zoned out
        return 10000 + (esp_random() % 10000);
    }
}

const TimeBasedAnimConfig* EyeState::selectBlinkSpeed(bool isDoubleBlink) {
    uint32_t variance = esp_random() % 100;

    if (variance < 15) {
        // 15% - Very fast (100ms) - surprised/alert
        return isDoubleBlink ? &TimeBasedAnimation::CONFIG_BLINK_VERY_FAST_DBL : &TimeBasedAnimation::CONFIG_BLINK_VERY_FAST;
    } else if (variance < 40) {
        // 25% - Fast (120ms) - active
        return isDoubleBlink ? &TimeBasedAnimation::CONFIG_BLINK_FAST_DBL : &TimeBasedAnimation::CONFIG_BLINK_FAST;
    } else if (variance < 75) {
        // 35% - Normal (150ms) - standard
        return isDoubleBlink ? &TimeBasedAnimation::CONFIG_BLINK_NORMAL_DBL : &TimeBasedAnimation::CONFIG_BLINK_NORMAL;
    } else if (variance < 90) {
        // 15% - Slow (200ms) - relaxed
        return isDoubleBlink ? &TimeBasedAnimation::CONFIG_BLINK_SLOW_DBL : &TimeBasedAnimation::CONFIG_BLINK_SLOW;
    } else {
        // 10% - Very slow (250ms) - tired/sleepy
        return isDoubleBlink ? &TimeBasedAnimation::CONFIG_BLINK_VERY_SLOW_DBL : &TimeBasedAnimation::CONFIG_BLINK_VERY_SLOW;
    }
}

void EyeState::blink() {
    const uint8_t* currentFrame = TimeBasedAnimation::update(blinkAnim);
    display->drawEye(currentFrame);

    // Between double-blink halves: hold the closed frame without blocking the render task
    if (blinkResumeAt != 0) {
        if ((long)(millis() - blinkResumeAt) >= 0) {
            blinkResumeAt = 0;
            TimeBasedAnimation::reset(blinkAnim);
        }
        return;
    }

    // Check if blink animation is complete
    if (TimeBasedAnimation::isComplete(blinkAnim)) {
        blinkCount++;

        // Check if we should do a second blink
        if (shouldDoubleBlink && blinkCount == 1) {
            // Varied pause between double blinks for more personality
            uint32_t pauseVariance = esp_random() % 100;
            unsigned long pauseDuration;

            if (pauseVariance < 30) {
                // 30% - Very quick double blink (50-100ms) - rapid/surprised
                pauseDuration = 50 + (esp_random() % 50);
            } else if (pauseVariance < 70) {
                // 40% - Normal double blink (100-150ms) - standard
                pauseDuration = 100 + (esp_random() % 50);
            } else {
                // 30% - Slow double blink (150-250ms) - thoughtful/deliberate
                pauseDuration = 150 + (esp_random() % 100);
            }

            blinkResumeAt = millis() + pauseDuration;
        } else {
            // Reset for next blink
            blinkCount = 0;
            shouldDoubleBlink = false;
            currentState = EyeStateEnum::IDLE;
        }
    }
}

void EyeState::playAnimationWithLoop(AnimationData& animData) {
    const uint8_t* currentFrame;

    if (isTransitioning) {
        // During transition: use full animation
        currentFrame = TimeBasedAnimation::update(animData.transitionAnim);

        // Also update loop animation in the background so it's in sync when we switch
        TimeBasedAnimation::update(animData.loopAnim);

        // Check if transition animation is complete
        if (TimeBasedAnimation::isComplete(animData.transitionAnim)) {
            isTransitioning = false;
            // Loop animation is already running in sync, no reset needed
        }
    } else {
        // After transition: loop only the last N frames
        currentFrame = TimeBasedAnimation::update(animData.loopAnim);
    }

    display->drawEye(currentFrame);
}

void EyeState::sleepFace() {
    unsigned long currentMillis = millis();
    unsigned long elapsedTime = currentMillis - startSleepTime;

    // Time it takes to fully close eyes (in milliseconds)
    const unsigned long FULL_CLOSE_DURATION = 30000;  // 30 seconds to fully close

    // Calculate target frame based on elapsed time (linear progression)
    uint8_t targetIndex = min((elapsedTime * sleepLength) / FULL_CLOSE_DURATION, (unsigned long)(sleepLength - 1));

    // Update frame at regular intervals with slight randomness for natural feel
    if (currentMillis >= nextSleepFrameChange) {
        // Gradually move toward target index (creates smooth, natural closing)
        if (sleepIndex < targetIndex) {
            sleepIndex++;
        } else if (sleepIndex > targetIndex && esp_random() % 100 < 30) {
            // 30% chance to occasionally open slightly (natural sleep movement)
            sleepIndex--;
        }

        // Next frame change in 300-500ms (natural variation)
        nextSleepFrameChange = currentMillis + 300 + (esp_random() % 200);
    }

    display->drawEye(eyeSleepAnimation[sleepIndex]);
}

void EyeState::resetSleepFace() {
    sleepIndex = 0;
    startSleepTime = millis();
    nextSleepFrameChange = millis();
}

void EyeState::renderGooglyEye() {
    display->drawEye(eyeGoogly);
    googlyEye.renderEye(sensorData.accelZ, sensorData.accelX);
    display->drawEyePupil(eyePupil, googlyEye.x, googlyEye.y);
}
