#include "KMMXController.h"
#include "MotionDetectionConfig.h"

// ============================================================================
// Motion Detection & Glitch Control Methods (BLE Interface)
// ============================================================================

/**
 * @brief Manually trigger glitch effect with specified intensity
 * @param intensity Glitch intensity (0-100)
 */
void KMMXController::triggerGlitch(uint8_t intensity) {
    // Constrain to valid range
    intensity = constrain(intensity, 0, 100);

    // Trigger glitch on display
    display.triggerGlitch(intensity);

    if (enableMotionDebug) {
        Serial.print(F("[BLE] Manual glitch triggered: intensity="));
        Serial.println(intensity);
    }
}

/**
 * @brief Set motion detection enable flags (bitfield)
 * Bit 0: Enable Tap Detection
 * Bit 1: Enable Petting Detection
 * Bit 2: Enable Tilt Detection
 * Bit 3: Enable Upside Down Detection
 * Bit 4: Enable Boop Detection
 * @param flags Bitfield of motion detection flags
 */
void KMMXController::setMotionEnableFlags(uint8_t flags) {
    enableTapDetection = (flags & 0x01) != 0;
    enablePettingDetection = (flags & 0x02) != 0;
    enableTiltDetection = (flags & 0x04) != 0;
    enableUpsideDownDetection = (flags & 0x08) != 0;
    enableBoopDetection = (flags & 0x10) != 0;

    if (enableMotionDebug) {
        Serial.print(F("[BLE] Motion flags: Tap="));
        Serial.print(enableTapDetection);
        Serial.print(F(" Pet="));
        Serial.print(enablePettingDetection);
        Serial.print(F(" Tilt="));
        Serial.print(enableTiltDetection);
        Serial.print(F(" Upside="));
        Serial.print(enableUpsideDownDetection);
        Serial.print(F(" Boop="));
        Serial.println(enableBoopDetection);
    }
}

/**
 * @brief Get current motion detection enable flags
 * @return Bitfield of motion detection flags
 */
uint8_t KMMXController::getMotionEnableFlags() {
    uint8_t flags = 0;
    if (enableTapDetection) flags |= 0x01;
    if (enablePettingDetection) flags |= 0x02;
    if (enableTiltDetection) flags |= 0x04;
    if (enableUpsideDownDetection) flags |= 0x08;
    if (enableBoopDetection) flags |= 0x10;
    return flags;
}

/**
 * @brief Set tap sensitivity (0-100)
 * Maps user-friendly 0-100 scale to tap spike threshold (0.2-5.0 m/s²)
 * 0 = very sensitive (0.2 m/s²), 100 = requires hard tap (5.0 m/s²)
 * @param sensitivity Sensitivity value 0-100
 */
void KMMXController::setTapSensitivity(uint8_t sensitivity) {
    // Constrain to valid range
    sensitivity = constrain(sensitivity, 0, 100);

    // Map 0-100 to 0.2-5.0 m/s² threshold
    // Lower sensitivity value = more sensitive (lower threshold)
    const float minThreshold = 0.2f;
    const float maxThreshold = 5.0f;
    tapSpikeThreshold = minThreshold + (sensitivity / 100.0f) * (maxThreshold - minThreshold);

    if (enableMotionDebug) {
        Serial.print(F("[BLE] Tap sensitivity: "));
        Serial.print(sensitivity);
        Serial.print(F(" (threshold="));
        Serial.print(tapSpikeThreshold);
        Serial.println(F(" m/s²)"));
    }
}

/**
 * @brief Get current tap sensitivity (0-100)
 * @return Sensitivity value 0-100
 */
uint8_t KMMXController::getTapSensitivity() {
    // Map threshold (0.2-5.0 m/s²) back to 0-100 scale
    const float minThreshold = 0.2f;
    const float maxThreshold = 5.0f;
    float normalized = (tapSpikeThreshold - minThreshold) / (maxThreshold - minThreshold);
    normalized = constrain(normalized, 0.0f, 1.0f);
    return (uint8_t)(normalized * 100.0f);
}

/**
 * @brief Set glitch intensity (0-100)
 * Controls the min/max intensity range for auto-triggered glitches
 * @param intensity Base intensity value 0-100
 */
void KMMXController::setGlitchIntensity(uint8_t intensity) {
    // Constrain to valid range
    intensity = constrain(intensity, 0, 100);

    // Set min/max intensity range centered around user value
    // Example: intensity=50 -> min=25, max=75 (±25 range)
    const int rangeSpread = 25;
    tapGlitchMinIntensity = constrain(intensity - rangeSpread, 0, 100);
    tapGlitchMaxIntensity = constrain(intensity + rangeSpread, 0, 100);

    if (enableMotionDebug) {
        Serial.print(F("[BLE] Glitch intensity: "));
        Serial.print(intensity);
        Serial.print(F(" (range="));
        Serial.print(tapGlitchMinIntensity);
        Serial.print(F("-"));
        Serial.print(tapGlitchMaxIntensity);
        Serial.println(F(")"));
    }
}

/**
 * @brief Get current glitch intensity (0-100)
 * Returns the center value of the min/max range
 * @return Intensity value 0-100
 */
uint8_t KMMXController::getGlitchIntensity() {
    // Return center of min/max range
    return (tapGlitchMinIntensity + tapGlitchMaxIntensity) / 2;
}
