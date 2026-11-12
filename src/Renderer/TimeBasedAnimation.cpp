#include "TimeBasedAnimation.h"
#include <math.h>

// Preset configurations
const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_SMOOTH_LOOP = {
    .durationMs = 900,
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 0,
    .pauseAtStartMs = 0,
    .easingType = EasingType::EASE_IN_OUT
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BREATHING = {
    .durationMs = 1500,      // Faster: 2000 -> 1500ms
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 300,     // Shorter pause: 500 -> 300ms
    .pauseAtStartMs = 400,   // Shorter pause: 800 -> 400ms
    .easingType = EasingType::BREATHING_NATURAL
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK = {
    .durationMs = 150,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 50,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_TRANSITION_END = {
    .durationMs = 250,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,
    .pauseAtStartMs = 0,
    .easingType = EasingType::EASE_IN_OUT
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_SMILE_LOOP = {
    .durationMs = 900,
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 300,
    .pauseAtStartMs = 12,
    .easingType = EasingType::EASE_IN_OUT
};

// Blink speed variants (for dynamic blink variance)
const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_VERY_FAST = {
    .durationMs = 100,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 30,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_VERY_FAST_DBL = {
    .durationMs = 100,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,  // No pause for double blink
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_FAST = {
    .durationMs = 120,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 40,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_FAST_DBL = {
    .durationMs = 120,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,  // No pause for double blink
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_NORMAL = {
    .durationMs = 150,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 50,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_NORMAL_DBL = {
    .durationMs = 150,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,  // No pause for double blink
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_SLOW = {
    .durationMs = 200,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 60,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_SLOW_DBL = {
    .durationMs = 200,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,  // No pause for double blink
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_VERY_SLOW = {
    .durationMs = 250,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 80,
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BLINK_VERY_SLOW_DBL = {
    .durationMs = 250,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,  // No pause for double blink
    .pauseAtStartMs = 0,
    .easingType = EasingType::NONE
};

// Mouth animation presets
const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_WAH = {
    .durationMs = 800,
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 200,
    .pauseAtStartMs = 200,
    .easingType = EasingType::EASE_IN_OUT
};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BREATHING_SLOW = {
    .durationMs = 3500,
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 1600,
    .pauseAtStartMs = 2200,
    .easingType = EasingType::BREATHING_NATURAL
};

// Smile transition preset
const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_SMILE_TRANSITION = {
    .durationMs = 700,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,
    .pauseAtStartMs = 0,
    .easingType = EasingType::EASE_IN_OUT
};

// New expressive animation presets
const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_BOUNCE_OVERSHOOT = {
    .durationMs = 1000,
    .playMode = AnimationPlayMode::PING_PONG,
    .pauseAtEndMs = 0,
    .pauseAtStartMs = 300,
    .easingType = EasingType::BOUNCE_OVERSHOOT};

const TimeBasedAnimConfig TimeBasedAnimation::CONFIG_ANTICIPATION = {
    .durationMs = 500,
    .playMode = AnimationPlayMode::ONCE,
    .pauseAtEndMs = 0,
    .pauseAtStartMs = 0,
    .easingType = EasingType::ANTICIPATION
};

void TimeBasedAnimation::init(TimeBasedAnimState& anim, const uint8_t** frames, short frameCount, const TimeBasedAnimConfig& config) {
    anim.frames = frames;
    anim.frameCount = frameCount;
    anim.currentFrameIndex = 0;
    anim.config = config;
    anim.animStartTime = millis();
    anim.pauseStartTime = 0;
    anim.isPaused = false;
    anim.isReversing = false;
    anim.hasCompleted = false;
}

void TimeBasedAnimation::reset(TimeBasedAnimState& anim, short startFrameIndex) {
    anim.currentFrameIndex = constrain(startFrameIndex, 0, anim.frameCount - 1);
    anim.animStartTime = millis() - (anim.frameCount > 1
        ? anim.config.durationMs * anim.currentFrameIndex / (anim.frameCount - 1)
        : 0);
    anim.pauseStartTime = 0;
    anim.isPaused = false;
    anim.isReversing = false;
    anim.hasCompleted = false;
}

void TimeBasedAnimation::setConfig(TimeBasedAnimState& anim, const TimeBasedAnimConfig& config) {
    anim.config = config;
    reset(anim);
}

bool TimeBasedAnimation::isComplete(const TimeBasedAnimState& anim) {
    return anim.hasCompleted;
}

const uint8_t* TimeBasedAnimation::update(TimeBasedAnimState& anim) {
    unsigned long currentTime = millis();

    // Handle completed animations
    if (anim.hasCompleted) {
        return anim.frames[anim.currentFrameIndex];
    }

    // Handle pause state
    if (anim.isPaused) {
        unsigned long pauseDuration = anim.isReversing ? anim.config.pauseAtStartMs : anim.config.pauseAtEndMs;
        if (currentTime - anim.pauseStartTime >= pauseDuration) {
            anim.isPaused = false;
            anim.animStartTime = currentTime;
        } else {
            return anim.frames[anim.currentFrameIndex];
        }
    }

    // Calculate elapsed time
    unsigned long elapsedMs = currentTime - anim.animStartTime;

    // Calculate target frame index
    short targetFrameIndex = calculateFrameIndex(anim, elapsedMs);

    // Frame skipping is always enabled - jump directly to target frame to maintain timing
    anim.currentFrameIndex = targetFrameIndex;

    // Check if we've reached the end of the animation cycle
    bool reachedEnd = false;
    if (anim.isReversing && anim.currentFrameIndex <= 0) {
        anim.currentFrameIndex = 0;
        reachedEnd = true;
    } else if (!anim.isReversing && anim.currentFrameIndex >= anim.frameCount - 1) {
        anim.currentFrameIndex = anim.frameCount - 1;
        reachedEnd = true;
    }

    // Handle end-of-cycle behavior
    if (reachedEnd) {
        switch (anim.config.playMode) {
            case AnimationPlayMode::ONCE:
                anim.hasCompleted = true;
                break;

            case AnimationPlayMode::LOOP:
                anim.currentFrameIndex = 0;
                anim.animStartTime = currentTime;
                break;

            case AnimationPlayMode::PING_PONG:
                anim.isReversing = !anim.isReversing;
                anim.isPaused = true;
                anim.pauseStartTime = currentTime;
                break;
        }
    }

    return anim.frames[anim.currentFrameIndex];
}

short TimeBasedAnimation::calculateFrameIndex(const TimeBasedAnimState& anim, unsigned long elapsedMs) {
    if (anim.frameCount <= 1) return 0;

    // Calculate linear progress (0.0 - 1.0)
    // A relaxed exhale takes longer than the inhale, avoiding a mechanical ping-pong rhythm.
    float durationMs = anim.config.durationMs;
    if (anim.config.easingType == EasingType::BREATHING_NATURAL && anim.isReversing) {
        durationMs *= 1.4f;
    }
    float progress = (float)elapsedMs / durationMs;

    // Clamp to valid range
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    // Apply easing based on type
    if (anim.config.easingType != EasingType::NONE) {
        progress = applyEasing(progress, anim.config.easingType);
    }

    // Convert progress to frame index
    // Note: All easing functions return values in [0.0, 1.0] range
    // Overshoot effects are built into the animation frames themselves
    short frameIndex;
    if (anim.isReversing) {
        frameIndex = (short)((1.0f - progress) * (anim.frameCount - 1));
    } else {
        frameIndex = (short)(progress * (anim.frameCount - 1));
    }

    // Clamp to valid frame range
    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= anim.frameCount) frameIndex = anim.frameCount - 1;

    return frameIndex;
}

float TimeBasedAnimation::applyEasing(float t, EasingType easingType) {
    switch (easingType) {
        case EasingType::NONE:
            return t;

        case EasingType::EASE_IN_OUT: {
            // Ease in-out cubic with fixed strength of 0.5
            // Provides a balanced smooth animation curve
            float eased;
            if (t < 0.5f) {
                eased = 4.0f * t * t * t;
            } else {
                float f = (2.0f * t - 2.0f);
                eased = 0.5f * f * f * f + 1.0f;
            }
            // Blend between linear and eased (50/50 blend)
            return t * 0.5f + eased * 0.5f;
        }

        case EasingType::BOUNCE_OVERSHOOT: {
            // Bounces to 50% first, then overshoots to 100% and settles
            // The overshoot is built into the animation frames (e.g., frame 47 of 48 is the overshoot position)
            // Creates that alive, reactive, bouncy feel!
            if (t < 0.33f) {
                // First third: accelerate to 50%
                float t2 = t / 0.33f;  // 0 to 1
                return 0.5f * t2 * t2;  // Quadratic ease to 0.5 (50%)
            } else if (t < 0.66f) {
                // Second third: bounce from 50% to 100% (overshoot)
                float t2 = (t - 0.33f) / 0.33f;  // 0 to 1
                return 0.5f + 0.5f * t2 * t2;  // Quadratic ease from 50% to 100%
            } else {
                // Final third: settle at 100% with slight ease
                float t2 = (t - 0.66f) / 0.34f;  // 0 to 1
                // Small bounce back and settle
                return 1.0f - 0.1f * (1.0f - t2) * (1.0f - t2);  // Slight settle to 100%
            }
        }

        case EasingType::ANTICIPATION: {
            // Pulls back slightly before launching forward
            // The pullback is built into the animation frames (early frames show the anticipation)
            // Great for reactions - adds personality!
            if (t < 0.2f) {
                // First 20%: ease into the pullback position (stays at ~0%)
                float t2 = t / 0.2f;  // 0 to 1
                return t2 * t2 * 0.1f;  // Quadratic ease to 10% (the pullback frame)
            } else {
                // Remaining 80%: launch forward to 100%
                float t2 = (t - 0.2f) / 0.8f;  // 0 to 1
                // Cubic ease out from 10% to 100%
                return 0.1f + 0.9f * (1.0f - pow(1.0f - t2, 3.0f));
            }
        }

        case EasingType::BREATHING_NATURAL: {
            // Quintic easing removes the visible acceleration changes of the old two-part curve.
            return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        }

        default:
            return t;
    }
}

