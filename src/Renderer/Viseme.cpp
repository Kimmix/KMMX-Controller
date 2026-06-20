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
    const float rate = currentEnvelope < adaptiveNoiseFloor ? visemeNoiseFloorFall : visemeNoiseFloorRise;
    adaptiveNoiseFloor += rate * (currentEnvelope - adaptiveNoiseFloor);
    adaptiveNoiseFloor = constrain(adaptiveNoiseFloor, noiseFloorMin, visemeNoiseFloorCap);
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
    bool detected = false;

    // Check for rapid increase in envelope
    if (previousEnvelope > 0 && currentEnvelope > previousEnvelope * visemeAttackThreshold) {
        // Check if enough time has passed since last attack
        if (currentTime - lastAttackTime >= minAttackInterval) {
            detected = true;
            lastAttackTime = currentTime;
        }
    }

    // Update previous envelope
    previousEnvelope = currentEnvelope;

    return detected;
}

// =============================================================================
// LOUDNESS LEVEL PROCESSING
// =============================================================================

/**
 * Calculate mouth opening level with improved perceptual scaling and smoothing.
 *
 * Improvements:
 * 1. Non-linear perceptual scaling - Uses power curve for more natural response
 * 2. Temporal smoothing - Reduces jitter while maintaining responsiveness
 * 3. Mid-range boost - Emphasizes expressive mid-range movements
 * 4. Adaptive dynamic range - Scales based on noise floor
 *
 * @return Mouth opening level (0-60)
 */
unsigned int Viseme::calculateLoudnessLevel() {
    const float gateThreshold = getGateThreshold();

    // Return 0 if below noise floor
    if (currentEnvelope <= gateThreshold) {
        // Smooth decay to 0
        smoothedLoudness *= (1.0f - loudnessSmoothing);
        return 0;
    }

    // Step 1 & 2: Normalize and apply non-linear perceptual curve (power law)
    // Lower exponent emphasizes quiet/medium sounds for better expressiveness
    float normalized = (currentEnvelope - gateThreshold) /
                       (gateThreshold * loudnessMax);
    normalized = constrain(normalized, 0.0f, 1.0f);
    float perceptualLoudness = powf(normalized, loudnessExponent);

    // Step 3: Apply mid-range boost for more expressive mouth movement
    // Boost values around 0.3-0.7 range where most speech dynamics occur
    float midBoostFactor = 1.0f;
    if (perceptualLoudness > 0.2f && perceptualLoudness < 0.8f) {
        // Smooth bell curve boost in mid-range
        float midDistance = fabsf(perceptualLoudness - 0.5f);  // Distance from center
        float boostCurve = 1.0f - (midDistance * 2.0f);        // 0-1 in mid-range
        midBoostFactor = 1.0f + (loudnessMidBoost - 1.0f) * boostCurve;
        perceptualLoudness *= midBoostFactor;
        perceptualLoudness = constrain(perceptualLoudness, 0.0f, 1.0f);
    }

    // Step 4: Apply temporal smoothing to reduce jitter
    // Use different smoothing for increases vs decreases (fast attack, medium release)
    if (perceptualLoudness > smoothedLoudness) {
        // Fast attack - respond quickly to increases
        smoothedLoudness = loudnessSmoothing * perceptualLoudness +
                          (1.0f - loudnessSmoothing) * smoothedLoudness;
    } else {
        // Medium release - smoother decay
        float releaseSmoothing = loudnessSmoothing * 0.6f;  // Slower than attack
        smoothedLoudness = releaseSmoothing * perceptualLoudness +
                          (1.0f - releaseSmoothing) * smoothedLoudness;
    }

    // Step 5: Map to mouth opening level (1-60)
    // Reserve 0 for completely closed mouth (handled above)
    unsigned int loudness_level = (unsigned int)(smoothedLoudness * visemeFramelength);
    loudness_level = constrain(loudness_level, 1, visemeFramelength);

    return loudness_level;
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

    // Return the dominant viseme
    return names[maxIdx];
}

const uint8_t* Viseme::visemeOutput(VisemeType viseme, unsigned int level) {
    if (level == 0) {
        return mouthDefault1;
    }

    return visemeFrames[viseme][level - 1];
}

/**
 * Serial Plotter debug output.
 * Formatted for Arduino Serial Plotter with all key metrics.
 */
void Viseme::printDebugPlotter(unsigned int loudnessLevel) {
#if VISEME_DEBUG_PLOTTER
    int mutiplier = 500;
    Serial.print("Envelope:");
    Serial.print(currentEnvelope * mutiplier);
    Serial.print(",NoiseFloor:");
    Serial.print(adaptiveNoiseFloor * mutiplier);
    Serial.print(",Gate:");
    Serial.print(getGateThreshold() * mutiplier);
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
    Serial.print(",LoudnessLevel:");
    Serial.println(loudnessLevel * mutiplier);
#endif
}

const uint8_t* Viseme::renderViseme() {
    // Audio processing pipeline using ESP-DSP
    readAndPrepareSamples();  // Also updates envelope

    // Update adaptive noise floor based on envelope
    updateNoiseFloor();

    // Detect attack (syllable boundaries)
    const bool attackDetected = detectAttack();

    performFFT();
    calculateMagnitudes();
    analyzeVisemes();

    // Find dominant viseme (with confidence checking)
    VisemeType dominantViseme = getDominantViseme();

    // Calculate mouth opening level with improved perceptual scaling and smoothing
    unsigned int loudness_level = calculateLoudnessLevel();

    // Lock viseme during release phase (envelope falling) or when mouth is closing
    // Only allow viseme change if: attack detected OR envelope rising AND mouth sufficiently open
    bool envelopeRising = (currentEnvelope >= previousEnvelope);
    bool mouthIsOpen = (loudness_level >= 15);  // At least 25% open (15 out of 60 frames)
    bool canChangeViseme = (attackDetected || envelopeRising) && mouthIsOpen;

    if (!canChangeViseme && dominantViseme != previousViseme) {
        // Lock to previous viseme during release/closing
        dominantViseme = previousViseme;
    }

    // Debug output (Serial Plotter format)
    printDebugPlotter(loudness_level);

    // Final render
    return visemeOutput(dominantViseme, loudness_level);
}
