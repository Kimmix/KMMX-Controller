#include "Viseme.h"
#include "VisemeConfig.h"
#include <cmath>

void Viseme::initMic() {
    mic.init(i2sSampleRate, i2sSamples);

    // Pre-compute Hann window for efficiency (~3% CPU savings per frame)
    initHannWindow();

    // Initialize ESP-DSP FFT tables
    if (!initFFT()) {
        Serial.println("ERROR: Failed to initialize ESP-DSP FFT!");
    }
}

float Viseme::getNoiseThreshold() {
    return adaptiveNoiseFloor;
}

void Viseme::setNoiseThreshold(float value) {
    adaptiveNoiseFloor = value;
}

// =============================================================================
// INITIALIZATION FUNCTIONS
// =============================================================================

/**
 * Pre-compute the Hann window coefficients.
 * This saves ~3% CPU per frame compared to computing on-the-fly.
 */
void Viseme::initHannWindow() {
    for (int i = 0; i < i2sSamples; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (i2sSamples - 1)));
    }
}

/**
 * Initialize ESP-DSP FFT tables.
 * Must be called once before performing FFT.
 */
bool Viseme::initFFT() {
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, i2sSamples);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: FFT init failed: %s\n", esp_err_to_name(ret));
        return false;
    }
    return true;
}

// =============================================================================
// AUDIO PROCESSING FUNCTIONS
// =============================================================================

/**
 * Read samples from I2S microphone and prepare for FFT.
 * Applies: DC removal -> Hann window
 *
 * IMPORTANT: Window is applied BEFORE copying to interleaved FFT buffer.
 * This fixes the bug in the original implementation where windowing was
 * applied to interleaved data, corrupting the imaginary components.
 */
void Viseme::readAndPrepareSamples() {
    // Read raw 16-bit samples from I2S
    mic.read(i2sBuffer, i2sSamples);

    // Calculate DC offset (mean value) for removal
    int32_t dcSum = 0;
    for (int i = 0; i < i2sSamples; i++) {
        dcSum += i2sBuffer[i];
    }
    float dcOffset = (float)dcSum / i2sSamples;

    // Apply DC removal, store in tempSamples for RMS calculation
    for (int i = 0; i < i2sSamples; i++) {
        tempSamples[i] = (float)i2sBuffer[i] - dcOffset;
    }

    // Update envelope tracker BEFORE windowing (more accurate amplitude)
    updateEnvelope();

    // Apply Hann window and copy to interleaved FFT buffer
    for (int i = 0; i < i2sSamples; i++) {
        float windowed = tempSamples[i] * hannWindow[i];
        fftBuffer[2 * i] = windowed;        // Real part
        fftBuffer[2 * i + 1] = 0.0f;        // Imaginary part = 0
    }
}

/**
 * Perform FFT using ESP-DSP library.
 * Uses hardware acceleration on ESP32-S3.
 */
void Viseme::performFFT() {
    // Perform in-place radix-2 FFT
    dsps_fft2r_fc32(fftBuffer, i2sSamples);

    // Bit-reverse the output for correct ordering
    dsps_bit_rev_fc32(fftBuffer, i2sSamples);
}

/**
 * Calculate magnitude spectrum from FFT output.
 * Only computes first half (positive frequencies, 0 to Nyquist).
 */
void Viseme::calculateMagnitudes() {
    for (int i = 0; i < i2sSamples / 2; i++) {
        float re = fftBuffer[2 * i];
        float im = fftBuffer[2 * i + 1];
        magnitudes[i] = sqrtf(re * re + im * im);
    }
}

/**
 * Convert FFT bin index to frequency in Hz.
 */
inline float Viseme::binToFrequency(int bin) {
    return bin * ((i2sSampleRate / 2.0f) / (i2sSamples / 2.0f));
}

/**
 * Analyze magnitude spectrum and compute viseme amplitudes.
 */
void Viseme::analyzeVisemes() {
    // Reset amplitudes
    ahAmplitude = eeAmplitude = ohAmplitude = ooAmplitude = thAmplitude = 0;

    for (int i = 0; i < i2sSamples / 2; i++) {
        float freq = binToFrequency(i);
        float mag = magnitudes[i];

        // Accumulate energy in each viseme frequency band
        if (freq >= visemeAhFreqMin && freq <= visemeAhFreqMax) {
            ahAmplitude += mag;
        }
        if (freq >= visemeEeFreqMin && freq <= visemeEeFreqMax) {
            eeAmplitude += mag;
        }
        if (freq >= visemeOhFreqMin && freq <= visemeOhFreqMax) {
            ohAmplitude += mag;
        }
        if (freq >= visemeOoFreqMin && freq <= visemeOoFreqMax) {
            ooAmplitude += mag;
        }
        if (freq >= visemeThFreqMin && freq <= visemeThFreqMax) {
            thAmplitude += mag;
        }
    }

    ahAmplitude *= ahScale;
    eeAmplitude *= eeScale;
    ohAmplitude *= ohScale;
    ooAmplitude *= ooScale;
    thAmplitude *= thScale;
}

// =============================================================================
// ENVELOPE TRACKING
// =============================================================================

/**
 * Calculate RMS (Root Mean Square) amplitude from current audio samples.
 * This provides a smoothed amplitude measurement of the audio signal.
 *
 * Note: RMS of DC-removed int16 samples typically ranges from:
 * - Silence: 1-5
 * - Quiet room: 5-15
 * - Normal speech: 20-100
 * The noise floor thresholds are calibrated to this scale.
 */
float Viseme::calculateRMS() {
    float sum = 0;
    for (int i = 0; i < i2sSamples; i++) {
        float sample = tempSamples[i];
        sum += sample * sample;
    }
    return sqrtf(sum / i2sSamples);
}

/**
 * Update envelope tracker with attack/release characteristics.
 * Fast attack (quick response to increases) and slow release (gradual decay).
 */
void Viseme::updateEnvelope() {
    float rms = calculateRMS();

    // Use attack coefficient for increases, release for decreases
    if (rms > currentEnvelope) {
        currentEnvelope = envelopeAttack * rms + (1.0f - envelopeAttack) * currentEnvelope;
    } else {
        currentEnvelope = envelopeRelease * rms + (1.0f - envelopeRelease) * currentEnvelope;
    }
}

// =============================================================================
// ADAPTIVE NOISE FLOOR
// =============================================================================

/**
 * Adaptive noise floor that automatically adjusts to ambient sound levels.
 * Adapts down during silence, adapts up during sustained noise.
 */
void Viseme::updateNoiseFloor() {
    // Check if currently speaking (envelope above threshold)
    bool isSpeaking = currentEnvelope > adaptiveNoiseFloor * 1.2f;

    if (isSpeaking) {
        // During speech, very slowly adapt upward if needed
        // This prevents brief loud sounds from permanently raising the floor
        if (currentEnvelope < adaptiveNoiseFloor * 0.8f) {
            adaptiveNoiseFloor -= noiseAdaptSpeed * 10.0f;
        }
    } else {
        // During silence, track the minimum amplitude
        if (currentEnvelope < adaptiveNoiseFloor) {
            // Quick adaptation down to new quiet level
            adaptiveNoiseFloor -= noiseAdaptSpeed * 50.0f;
        } else {
            // Slow adaptation up toward current noise level
            adaptiveNoiseFloor += noiseAdaptSpeed;
        }
    }

    // Clamp to min/max range
    if (adaptiveNoiseFloor < noiseFloorMin) {
        adaptiveNoiseFloor = noiseFloorMin;
    }
    if (adaptiveNoiseFloor > noiseFloorMax) {
        adaptiveNoiseFloor = noiseFloorMax;
    }
}

// =============================================================================
// ATTACK DETECTION
// =============================================================================

/**
 * Detect attack (rapid envelope increase) indicating syllable boundaries.
 * Returns true when a valid attack is detected.
 */
bool Viseme::detectAttack() {
    unsigned long currentTime = millis();
    attackDetected = false;

    // Check for rapid increase in envelope
    if (previousEnvelope > 0 && currentEnvelope > previousEnvelope * attackThreshold) {
        // Check if enough time has passed since last attack
        if (currentTime - lastAttackTime >= minAttackInterval) {
            attackDetected = true;
            lastAttackTime = currentTime;
        }
    }

    // Update previous envelope
    previousEnvelope = currentEnvelope;

    return attackDetected;
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * Determine the dominant viseme based on highest amplitude.
 * Requires the dominant viseme to be clearly separated from second place.
 * Implements confidence tracking for stability.
 */
Viseme::VisemeType Viseme::getDominantViseme() {
    // Find max and second max
    float amps[5] = {ahAmplitude, eeAmplitude, ohAmplitude, ooAmplitude, thAmplitude};
    VisemeType names[5] = {AH, EE, OH, OO, TH};

    int maxIdx = 0;
    float maxAmp = amps[0];
    for (int i = 1; i < 5; i++) {
        if (amps[i] > maxAmp) {
            maxAmp = amps[i];
            maxIdx = i;
        }
    }

    // Find second highest
    float secondMax = 0;
    for (int i = 0; i < 5; i++) {
        if (i != maxIdx && amps[i] > secondMax) {
            secondMax = amps[i];
        }
    }

    // Check if amplitude is above adaptive noise floor
    if (maxAmp < adaptiveNoiseFloor) {
        return previousViseme;
    }

    // Check if there's sufficient separation (e.g. 10% stronger than 2nd place)
    if (maxAmp < secondMax * minSeparation) {
        return previousViseme;
    }

    // Return the dominant viseme
    return names[maxIdx];
}

const uint8_t* Viseme::visemeOutput(VisemeType viseme, unsigned int level) {
    if (level == 0) {
        return mouthDefault;
    }

    // Convert to 0-based index
    level -= 1;

    switch (viseme) {
        case AH:
            return ahViseme[level];
        case EE:
            return eeViseme[level];
        case OH:
            return ohViseme[level];
        case OO:
            return ooViseme[level];
        case TH:
            return thViseme[level];
    };
    return nullptr;
}

/**
 * Serial Plotter debug output.
 * Formatted for Arduino Serial Plotter with all key metrics.
 */
void Viseme::printDebugPlotter(float distinctiveness, unsigned int loudnessLevel, bool canChange) {
#if VISEME_DEBUG_PLOTTER
    int mutiplier = 500;
    Serial.print("Envelope:");
    Serial.print(currentEnvelope * mutiplier);
    Serial.print(",NoiseFloor:");
    Serial.print(adaptiveNoiseFloor * mutiplier);
    Serial.print(",AH:");
    Serial.print(ahAmplitude);
    Serial.print(",EE:");
    Serial.print(eeAmplitude);
    Serial.print(",OH:");
    Serial.print(ohAmplitude);
    Serial.print(",OO:");
    Serial.print(ooAmplitude);
    Serial.print(",TH:");
    Serial.print(thAmplitude);
    Serial.print(",Distinctiveness:");
    Serial.print(distinctiveness);
    Serial.print(",LoudnessLevel:");
    Serial.print(loudnessLevel * 500);  // Scale up for visibility
    Serial.print(",CanChange:");
    Serial.println(canChange ? 5000 : 0);  // High when unlocked, low when locked
#endif
}

const uint8_t* Viseme::renderViseme() {
    // Audio processing pipeline using ESP-DSP
    readAndPrepareSamples();  // Also updates envelope

    // Update adaptive noise floor based on envelope
    updateNoiseFloor();

    // Detect attack (syllable boundaries)
    detectAttack();

    performFFT();
    calculateMagnitudes();
    analyzeVisemes();

    const float maxAmplitude = max(max(ahAmplitude, eeAmplitude), max(max(ohAmplitude, ooAmplitude), thAmplitude));
    const float minAmplitude = min(min(ahAmplitude, eeAmplitude), min(min(ohAmplitude, ooAmplitude), thAmplitude));

    // Find dominant viseme (with confidence checking)
    VisemeType dominantViseme = getDominantViseme();

    // Calculate mouth opening level based on envelope (volume)
    unsigned int loudness_level = 0;
    float distinctiveness = maxAmplitude - minAmplitude;

    if (currentEnvelope > adaptiveNoiseFloor) {
        // Map envelope to mouth opening level (1-20)
        // Louder speech = bigger mouth opening
        loudness_level = (unsigned int)mapFloat(currentEnvelope, adaptiveNoiseFloor, adaptiveNoiseFloor * 3.0f, 1, visemeFramelength);
        loudness_level = constrain(loudness_level, 1, visemeFramelength);
    }

    // Lock viseme during release phase (envelope falling) or when mouth is closing
    // Only allow viseme change if: attack detected OR envelope rising AND mouth sufficiently open
    bool envelopeRising = (currentEnvelope >= previousEnvelope);
    bool mouthIsOpen = (loudness_level >= 5);  // At least 25% open (5 out of 20 frames)
    bool canChangeViseme = (attackDetected || envelopeRising) && mouthIsOpen;

    if (!canChangeViseme && dominantViseme != previousViseme) {
        // Lock to previous viseme during release/closing
        dominantViseme = previousViseme;
    }

    // Debug output (Serial Plotter format)
    printDebugPlotter(distinctiveness, loudness_level, canChangeViseme);

    // Final render
    return visemeOutput(dominantViseme, loudness_level);
}
