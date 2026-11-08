#include "KMMXController.h"
#include "MotionDetectionConfig.h"
#include <cmath>

/**
 * ========================================================================
 * MOTION DETECTION HELPER FUNCTIONS
 * ========================================================================
 */

/**
 * Check if enough time has passed since last event (debounce helper)
 * NOTE: This is overflow-safe! Unsigned subtraction handles millis() wraparound correctly.
 * Example: If millis() wraps from 0xFFFFFFFF to 0x00000001, and lastTime was 0xFFFFFFFE,
 * the subtraction (0x00000001 - 0xFFFFFFFE) = 3, which is correct elapsed time.
 */
inline bool KMMXController::hasDebounceExpired(unsigned long lastTime, uint16_t debounceTime) const {
    return (millis() - lastTime) >= debounceTime;
}

/**
 * Restore previous eye and mouth state (reusable across all detectors)
 */
void KMMXController::restorePreviousState(EyeStateEnum prevEye, MouthStateEnum prevMouth) {
    if (prevEye == EyeStateEnum::BOOP) prevEye = EyeStateEnum::IDLE;
    if (prevMouth == MouthStateEnum::BOOP || prevMouth == MouthStateEnum::ANGRYBOOP) prevMouth = MouthStateEnum::IDLE;
    eyeState.setState(prevEye, false, 0);      // Temporary, no timeout (manual control)
    mouthState.setState(prevMouth, false, 0);  // Temporary, no timeout (manual control)
}

/**
 * Reset tilt detector to neutral state (eliminates code duplication)
 */
void KMMXController::resetTiltToNeutral(unsigned long currentTime, bool wasForwardBack) {
    tiltDetector.isTilted = false;
    tiltDetector.tiltStartTime = 0;
    tiltDetector.lastTiltChangeTime = currentTime;
    tiltDetector.lastNeutralReturnTime = currentTime;

    if (wasForwardBack) {
        tiltDetector.lastForwardBackTime = currentTime;
    }

    restorePreviousState(tiltDetector.previousEyeState, tiltDetector.previousMouthState);

    if (enableMotionDebug) {
        Serial.printf("[TILT] Returned to neutral (was %s)\n",
                      wasForwardBack ? "Forward/Back" : "Left/Right");
    }
}

/**
 * Check if tilt direction can be switched (with cooldown enforcement)
 */
bool KMMXController::canSwitchTiltDirection(unsigned long currentTime) {
    unsigned long timeSinceLastChange = currentTime - tiltDetector.lastTiltChangeTime;

    if (timeSinceLastChange < tiltDirectionChangeCooldown) {
        if (enableMotionDebug) {
            Serial.printf("[TILT] Direction change blocked - cooldown active (%lu ms / %u ms)\n",
                          timeSinceLastChange, tiltDirectionChangeCooldown);
        }
        return false;
    }
    return true;
}

/**
 * Handle tilt state when currently tilted (active state)
 */
void KMMXController::handleActiveTiltState(unsigned long currentTime, bool isNeutral,
                                           bool isTiltedForwardBack, bool isTiltedLeftRight,
                                           float tiltX, float tiltZ) {
    // Check for return to neutral
    if (isNeutral) {
        resetTiltToNeutral(currentTime, !tiltDetector.isLeftRight);
        return;
    }

    // Forward/Back mode (with strict lock)
    if (!tiltDetector.isLeftRight) {
        // Block Left/Right switching
        if (isTiltedLeftRight && !isTiltedForwardBack) {
            if (enableMotionDebug) {
                Serial.println("[TILT] Left/Right blocked - Forward/Back is active (return to neutral first)");
            }
        }
        // Continue maintaining Forward/Back state
        return;
    }

    // Left/Right mode - allow switching to Forward/Back with cooldown
    if (isTiltedForwardBack && canSwitchTiltDirection(currentTime)) {
        tiltDetector.isLeftRight = false;
        tiltDetector.lastTiltChangeTime = currentTime;
        tiltDetector.lastForwardBackTime = currentTime;

        if (enableMotionDebug) {
            Serial.printf("[TILT] ✓ Direction changed! Angle: %.2f, Direction: Forward/Back\n", tiltX);
        }
        triggerTiltResponse(tiltX, false);
    }
}

/**
 * Handle tilt tracking when not currently tilted
 */
void KMMXController::handleTiltTracking(unsigned long currentTime, bool isNeutral,
                                        bool isTiltedForwardBack, bool isTiltedLeftRight,
                                        float tiltX, float tiltZ) {
    // Reset tracking if neutral
    if (isNeutral) {
        if (tiltDetector.tiltStartTime != 0) {
            tiltDetector.tiltStartTime = 0;  // Reset tracking silently
        }
        return;
    }

    // Determine which direction to track (Forward/Back has priority)
    bool trackForwardBack = isTiltedForwardBack;
    bool trackLeftRight = isTiltedLeftRight && !isTiltedForwardBack;

    if (!trackForwardBack && !trackLeftRight) {
        return;  // No significant tilt detected
    }

    // Start tracking on first detection
    if (tiltDetector.tiltStartTime == 0) {
        tiltDetector.tiltStartTime = currentTime;
        tiltDetector.tiltAngleX = tiltX;
        tiltDetector.tiltAngleZ = tiltZ;
        return;
    }

    // Check if sustained long enough
    if (currentTime - tiltDetector.tiltStartTime >= tiltSustainTime) {
        // Activate tilt response
        tiltDetector.isTilted = true;
        tiltDetector.isLeftRight = trackLeftRight;
        tiltDetector.previousEyeState = eyeState.getState();
        tiltDetector.previousMouthState = mouthState.getState();
        tiltDetector.lastTiltChangeTime = currentTime;

        if (trackForwardBack) {
            tiltDetector.lastForwardBackTime = currentTime;
        }

        float angle = trackLeftRight ? tiltZ : tiltX;
        const char* direction = trackLeftRight ? "Left/Right" : "Forward/Back";

        if (enableMotionDebug) {
            Serial.printf("[TILT] ✓ Motion detected! Angle: %.2f, Direction: %s (threshold: %.2f)\n",
                          angle, direction, tiltThreshold);
        }

        triggerTiltResponse(angle, trackLeftRight);
    }
}

/**
 * ========================================================================
 * MAIN MOTION DETECTION FUNCTIONS
 * ========================================================================
 */

/**
 * Main motion detection dispatcher - called from sensor task
 * Checks all motion features in priority order
 */
void KMMXController::checkMotionFeatures(KMMXController* controller) {
    // Log when motion detection becomes active (once)
    static bool motionDetectionStarted = false;
    if (!motionDetectionStarted && enableMotionDebug) {
        motionDetectionStarted = true;
        Serial.println("[MOTION] Motion detection is now active");
    }

    // Get current sensor data from active buffer
    const SensorData& current = controller->sensorBuffer[controller->activeBuffer];

    // Get current facial states
    EyeStateEnum currentEyeState = controller->eyeState.getState();
    MouthStateEnum currentMouthState = controller->mouthState.getState();

    // Check if currently in IDLE state (allows new motion detection to trigger)
    bool isFullyIdle = (currentEyeState == EyeStateEnum::IDLE && currentMouthState == MouthStateEnum::IDLE);

    // Upside-down detection (always runs - highest priority, can interrupt anything)
    if (enableUpsideDownDetection) {
        controller->detectUpsideDown(current);
        if (controller->upsideDownDetector.isUpsideDown) return;
    }

    // Tilt detection (runs if already active OR if fully idle)
    // When active: runs cleanup logic to restore state when returning to neutral
    // When idle: can trigger new tilt responses
    if (enableTiltDetection) {
        bool shouldRunTilt = controller->tiltDetector.isTilted || isFullyIdle;
        if (shouldRunTilt) {
            controller->detectTilt(current);
            if (controller->tiltDetector.isTilted) return;
        }
    }

    // Petting detection (runs if already active OR if fully idle)
    // When active: runs happiness decay and state restoration logic
    // When idle: can trigger new petting responses
    if (enablePettingDetection) {
        bool shouldRunPetting = controller->pettingDetector.isPetting || isFullyIdle;
        if (shouldRunPetting) {
            controller->detectPetting(current);
        }
    }

    // Tap detection (always runs when enabled - triggers glitch effect)
    if (enableTapDetection) {
        controller->detectTap(current);
    }
}

/**
 * Tilt Detection - REFACTORED for reduced complexity and better performance
 * Detects sustained head tilt for curious/confused expressions
 * Uses accelerometer axes to detect tilt angle and direction
 */
void KMMXController::detectTilt(const SensorData& current) {
    unsigned long currentTime = millis();

    // Cache tilt values
    float tiltX = current.accelX;  // Forward/back tilt
    float tiltZ = current.accelZ;  // Left/right tilt

    // Cache absolute values (optimization: avoid redundant fabsf() calls)
    float absTiltX = fabsf(tiltX);
    float absTiltZ = fabsf(tiltZ);

    // Determine physical state using cached values
    bool isTiltedForwardBack = absTiltX > tiltThreshold;
    bool isTiltedLeftRight = absTiltZ > tiltThreshold;
    bool isNeutral = (absTiltX < tiltNeutralThreshold && absTiltZ < tiltNeutralThreshold);

    // Early exit: debounce check
    if (!hasDebounceExpired(tiltDetector.lastTiltChangeTime, tiltDebounceTime)) {
        return;
    }

    // ========== STATE 1: Currently Tilted ==========
    if (tiltDetector.isTilted) {
        handleActiveTiltState(currentTime, isNeutral, isTiltedForwardBack, isTiltedLeftRight,
                              tiltX, tiltZ);
        return;
    }

    // ========== STATE 2: Not Currently Tilted ==========
    handleTiltTracking(currentTime, isNeutral, isTiltedForwardBack, isTiltedLeftRight,
                       tiltX, tiltZ);
}

/**
 * Trigger tilt response based on angle and direction
 */
void KMMXController::triggerTiltResponse(float angle, bool isLeftRight) {
    if (isLeftRight) {
        // Left/Right tilt - curious/confused
        eyeState.setState(EyeStateEnum::DOUBTED, false, 0);  // Temporary, no timeout (manual control)
        mouthState.setState(MouthStateEnum::EH, false, 0);   // Temporary, no timeout (manual control)
        statusLED.setColor(Color::MAGENTA);                  // Using MAGENTA instead of PURPLE

        if (enableMotionDebug) {
            Serial.printf("[TILT] Left/Right tilt response! Angle: %.2f\n", angle);
        }
    } else {
        // Forward/Back tilt
        if (angle > 0) {
            // Forward tilt (looking down) - unimpressed
            eyeState.setState(EyeStateEnum::UNIMPRESSED, false, 0);  // Temporary, no timeout (manual control)
            mouthState.setState(MouthStateEnum::IDLE, false, 0);     // Temporary, no timeout (manual control)
            statusLED.setColor(Color::BLUE);

            if (enableMotionDebug) {
                Serial.printf("[TILT] Forward tilt response! Angle: %.2f\n", angle);
            }
        } else {
            // Back tilt (looking up) - curious/surprised
            eyeState.setState(EyeStateEnum::ROUNDED, false, 0);   // Temporary, no timeout (manual control)
            mouthState.setState(MouthStateEnum::IDLE, false, 0);  // Temporary, no timeout (manual control)
            statusLED.setColor(Color::CYAN);

            if (enableMotionDebug) {
                Serial.printf("[TILT] Back tilt response! Angle: %.2f\n", angle);
            }
        }
    }
}

/**
 * Upside Down Detection - Detects when the character is held upside down
 * Uses Y-axis accelerometer to detect upside-down orientation
 */
void KMMXController::detectUpsideDown(const SensorData& current) {
    unsigned long currentTime = millis();
    float yAxis = current.accelY;
    bool isFlipped = yAxis > -upsideDownThreshold;

    // Early exit: debounce check
    if (!hasDebounceExpired(upsideDownDetector.lastStateChangeTime, upsideDownDebounceTime)) {
        return;
    }

    // ========== STATE 1: Currently Upside Down ==========
    if (upsideDownDetector.isUpsideDown) {
        // Check if returning to normal orientation
        if (!isFlipped) {
            // Return to normal
            upsideDownDetector.isUpsideDown = false;
            upsideDownDetector.upsideDownStartTime = 0;
            upsideDownDetector.lastStateChangeTime = currentTime;

            restorePreviousState(upsideDownDetector.previousEyeState,
                                 upsideDownDetector.previousMouthState);

            if (enableMotionDebug) {
                Serial.println("[UPSIDE DOWN] Returned to normal orientation");
            }
        }
        // If still upside down, maintain crying state (no action needed)
        return;
    }

    // ========== STATE 2: Not Currently Upside Down ==========
    if (!isFlipped) {
        // Normal orientation - reset any tracking
        if (upsideDownDetector.upsideDownStartTime != 0) {
            upsideDownDetector.upsideDownStartTime = 0;  // Reset tracking silently
        }
        return;
    }

    // Upside down detected - start or continue tracking
    if (upsideDownDetector.upsideDownStartTime == 0) {
        // First time detecting upside down - start tracking
        upsideDownDetector.upsideDownStartTime = currentTime;
        if (enableMotionDebug) {
            Serial.printf("[UPSIDE DOWN] Starting tracking... Y-axis: %.2f (threshold: %.2f)\n",
                          yAxis, upsideDownThreshold);
        }
        return;
    }

    // Already tracking - check if sustained long enough
    if (currentTime - upsideDownDetector.upsideDownStartTime >= upsideDownSustainTime) {
        // Trigger upside down response (crying)
        upsideDownDetector.isUpsideDown = true;
        upsideDownDetector.previousEyeState = eyeState.getState();
        upsideDownDetector.previousMouthState = mouthState.getState();
        upsideDownDetector.lastStateChangeTime = currentTime;

        if (enableMotionDebug) {
            Serial.printf("[UPSIDE DOWN] ✓ Upside down detected! Y-axis: %.2f (threshold: %.2f)\n",
                          yAxis, upsideDownThreshold);
        }

        triggerUpsideDownResponse();
    }
}

/**
 * Trigger upside down response - crying
 */
void KMMXController::triggerUpsideDownResponse() {
    eyeState.setState(EyeStateEnum::CRY, false, 0);      // Temporary, no timeout (manual control)
    mouthState.setState(MouthStateEnum::WAH, false, 0);  // Temporary, no timeout (manual control)
    statusLED.setColor(Color::BLUE);

    if (enableMotionDebug) {
        Serial.println("[UPSIDE DOWN] Crying response triggered!");
    }
}

/**
 * Petting Detection - Dynamic happiness accumulation system
 * Each pat adds happiness which decays over time. More natural and responsive than fixed time windows.
 */
void KMMXController::detectPetting(const SensorData& current) {
    unsigned long currentTime = millis();

    // Check if we're in cooldown after tilt returns to neutral
    // Cooldown activates when ANY tilt (forward/back or left/right) returns to neutral
    if (enablePettingCooldownAfterTilt) {
        unsigned long timeSinceNeutral = currentTime - tiltDetector.lastNeutralReturnTime;
        if (timeSinceNeutral < pettingCooldownAfterTilt) {
            // Still in cooldown period after returning to neutral - skip petting detection
            if (enableMotionDebug && pettingDetector.isPetting) {
                Serial.printf("[PETTING] Cooldown active - %lu ms since neutral return (need %u ms)\n",
                              timeSinceNeutral, pettingCooldownAfterTilt);
            }
            return;
        }
    }

    // ========== Initialize last update time on first run ==========
    if (pettingDetector.lastUpdateTime == 0) {
        pettingDetector.lastUpdateTime = currentTime;
    }

    // ========== Calculate happiness decay ==========
    // Happiness naturally decays over time (like a leaky bucket)
    float deltaTime = (currentTime - pettingDetector.lastUpdateTime) / 1000.0f;  // Convert to seconds
    pettingDetector.lastUpdateTime = currentTime;

    if (deltaTime > 0 && deltaTime < pettingDeltaTimeMax) {  // Sanity check (ignore huge time jumps)
        float decay = pettingHappinessDecayRate * deltaTime;
        pettingDetector.happiness -= decay;
        // Clamp to 0 using fmaxf (branchless, more efficient)
        pettingDetector.happiness = fmaxf(pettingDetector.happiness, 0.0f);
    }

    // ========== Spike Detection ==========
    // Calculate magnitude change (derivative)
    float magnitudeChange = fabsf(current.accelMagnitude - pettingDetector.lastMagnitude);
    pettingDetector.lastMagnitude = current.accelMagnitude;

    // Detect spike: sudden increase in acceleration magnitude
    bool spikeDetected = (magnitudeChange >= pettingSpikeThreshold);

    // Check spike cooldown to prevent double-counting
    if (spikeDetected && hasDebounceExpired(pettingDetector.lastSpikeTime, pettingSpikeCooldown)) {
        // Valid spike detected! Add happiness
        pettingDetector.lastSpikeTime = currentTime;
        pettingDetector.happiness += pettingHappinessPerPat;

        // Cap happiness at 100 using fminf (branchless, more efficient)
        pettingDetector.happiness = fminf(pettingDetector.happiness, 100.0f);

        if (enableMotionDebug) {
            Serial.printf("[PETTING] Pat detected! Change: %.2f m/s² | Happiness: %.1f/%.1f\n",
                          magnitudeChange, pettingDetector.happiness, pettingHappinessTrigger);
        }
    }

    // ========== Check happiness thresholds ==========
    if (!pettingDetector.isPetting) {
        // Not currently showing response - check if happiness reached trigger threshold
        if (pettingDetector.happiness >= pettingHappinessTrigger) {
            // Trigger petting response!
            pettingDetector.isPetting = true;
            pettingDetector.previousEyeState = eyeState.getState();
            pettingDetector.previousMouthState = mouthState.getState();

            if (enableMotionDebug) {
                Serial.printf("[PETTING] ✓ Petting triggered! Happiness: %.1f | Saved previous state: Eye=%d, Mouth=%d\n",
                              pettingDetector.happiness,
                              pettingDetector.previousEyeState,
                              static_cast<int>(pettingDetector.previousMouthState));
            }

            triggerPettingResponse();
        }
    } else {
        // Currently showing response - check if happiness dropped below end threshold
        if (pettingDetector.happiness < pettingHappinessEndThreshold) {
            // End petting response (happiness decayed naturally)
            pettingDetector.isPetting = false;
            pettingDetector.happiness = 0;  // Reset happiness

            // Return to previous state using helper
            restorePreviousState(pettingDetector.previousEyeState, pettingDetector.previousMouthState);

            if (enableMotionDebug) {
                Serial.printf("[PETTING] Response ended naturally (happiness decayed to %.1f) - returning to previous state (Eye: %d, Mouth: %d)\n",
                              pettingDetector.happiness,
                              pettingDetector.previousEyeState, pettingDetector.previousMouthState);
            }
        }
    }
}

/**
 * Trigger petting response - simplified to single SMILE state
 */
void KMMXController::triggerPettingResponse() {
    // Petting response - happy SMILE
    eyeState.setState(EyeStateEnum::SMILE, false, 0);     // Temporary, no timeout (manual control)
    mouthState.setState(MouthStateEnum::IDLE, false, 0);  // Temporary, no timeout (manual control)
    cheekPanel.setBrightness(200);
    statusLED.setColor(Color::PINK);

    if (enableMotionDebug) {
        Serial.println("[PETTING] Petting response! (SMILE)");
    }
}

/**
 * Tap Detection - Detects light taps for glitch effects
 * Uses two-stage peak + decay verification to distinguish taps from sustained motion
 *
 * Algorithm:
 * 1. STAGE 1: Detect initial spike above threshold
 * 2. STAGE 2: Wait for decay - verify magnitude drops quickly (confirming impulse, not sustained motion)
 */
void KMMXController::detectTap(const SensorData& current) {
    unsigned long currentTime = millis();

    // Calculate magnitude change (derivative)
    float magnitudeChange = fabsf(current.accelMagnitude - tapDetector.lastMagnitude);
    tapDetector.lastMagnitude = current.accelMagnitude;

    // ========== STAGE 1: Detect Initial Spike ==========
    if (!tapDetector.waitingForDecay) {
        // Check if magnitude change exceeds threshold (potential tap)
        if (magnitudeChange >= tapSpikeThreshold && hasDebounceExpired(tapDetector.lastTapTime, tapCooldown)) {
            // Potential tap detected! Start peak tracking
            tapDetector.peakMagnitude = magnitudeChange;
            tapDetector.peakTime = currentTime;
            tapDetector.waitingForDecay = true;

            if (enableMotionDebug) {
                Serial.printf("[TAP] Peak detected: %.2f m/s² - waiting for decay...\n", magnitudeChange);
            }
        }
        return;
    }

    // ========== STAGE 2: Verify Decay ==========
    unsigned long timeSincePeak = currentTime - tapDetector.peakTime;

    // Check if still within decay verification window
    if (timeSincePeak < tapPeakDecayWindow) {
        // Verify magnitude has dropped significantly (confirming it's a tap, not sustained motion)
        if (magnitudeChange < tapDetector.peakMagnitude * tapDecayRatio) {
            // Valid tap confirmed! Quick rise and fall = impulse
            tapDetector.waitingForDecay = false;
            tapDetector.lastTapTime = currentTime;

            if (enableMotionDebug) {
                Serial.printf("[TAP] ✓ Tap confirmed! Peak: %.2f m/s², Decay: %.2f m/s² (%.0f%% drop)\n",
                              tapDetector.peakMagnitude, magnitudeChange,
                              (1.0f - magnitudeChange / tapDetector.peakMagnitude) * 100.0f);
            }

            triggerTapResponse(tapDetector.peakMagnitude);
        }
        // Still within window but no decay yet - keep waiting
    } else {
        // Timeout - decay didn't happen fast enough, not a tap (sustained motion)
        tapDetector.waitingForDecay = false;

        if (enableMotionDebug) {
            Serial.printf("[TAP] ✗ Timeout - not a tap (sustained motion). Peak: %.2f m/s², Current: %.2f m/s²\n",
                          tapDetector.peakMagnitude, magnitudeChange);
        }
    }
}

/**
 * Trigger tap response - glitch effect on display
 * Scales intensity based on tap magnitude (duration is calculated internally by triggerGlitch)
 */
void KMMXController::triggerTapResponse(float tapMagnitude) {
    // Clamp tap magnitude to defined range
    float clampedMagnitude = constrain(tapMagnitude, tapMagnitudeMin, tapMagnitudeMax);

    // Calculate normalized tap strength (0.0 to 1.0)
    float tapStrength = (clampedMagnitude - tapMagnitudeMin) / (tapMagnitudeMax - tapMagnitudeMin);

    // Scale intensity based on tap strength
    int intensity = tapGlitchMinIntensity + (int)(tapStrength * (tapGlitchMaxIntensity - tapGlitchMinIntensity));

    // Store tap magnitude for HUD display
    tapDetector.lastTapMagnitude = tapMagnitude;
    tapDetector.lastTapDisplayTime = millis();

    // Trigger glitch with intensity only (duration calculated internally)
    display.triggerGlitch(intensity);

    if (enableMotionDebug) {
        Serial.printf("[TAP] Glitch effect triggered! Magnitude: %.2f m/s² | Strength: %.2f | Intensity: %d\n", tapMagnitude, tapStrength, intensity);
    }
}
