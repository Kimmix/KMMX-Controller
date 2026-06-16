#include "Viseme.h"
#include "VisemeConfig.h"
#include <vector>
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
    return noiseThreshold;
}

void Viseme::setNoiseThreshold(float value) {
    noiseThreshold = value;
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
 * Applies: DC removal -> smoothing -> Hann window
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

    // Apply DC removal, exponential smoothing, and Hann window
    float smoothedValue = 0;
    for (int i = 0; i < i2sSamples; i++) {
        // Remove DC offset
        float sample = (float)i2sBuffer[i] - dcOffset;

        // Apply exponential smoothing (low-pass filter)
        smoothedValue = alpha * sample + (1.0f - alpha) * smoothedValue;

        // Apply pre-computed Hann window
        tempSamples[i] = smoothedValue * hannWindow[i];
    }

    // Copy windowed samples to interleaved FFT buffer
    for (int i = 0; i < i2sSamples; i++) {
        fftBuffer[2 * i] = tempSamples[i];      // Real part
        fftBuffer[2 * i + 1] = 0.0f;            // Imaginary part = 0
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

    // Apply normalization to balance sensitivity across visemes
    normalizeViseme(ahAmplitude, eeAmplitude, ohAmplitude, ooAmplitude, thAmplitude);
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void Viseme::calculateAmplitude(float ah, float ee, float oh, float oo, float th, float& minAmp, float& maxAmp, float& avgAmp) {
    minAmp = min(min(ah, ee), min(min(oh, oo), th));
    maxAmp = max(max(ah, ee), max(max(oh, oo), th));
    avgAmp = (ah + ee + oh + oo + th) / 5.0f;
}

void Viseme::normalizeViseme(float& ah_amplitude, float& ee_amplitude, float& oh_amplitude, float& oo_amplitude, float& th_amplitude) {
    // Updated normalization factors from tested implementation
    ah_amplitude *= visemeAhScale;
    ee_amplitude *= visemeEeScale;
    oh_amplitude *= visemeOhScale;
    oo_amplitude *= visemeOoScale;
    th_amplitude *= visemeThScale;
}

void Viseme::levelBoost(Viseme::VisemeType viseme, float& maxAmp) {
    if (viseme != AH) {
        maxAmp *= 1.5f;
    }
}

unsigned int Viseme::calculateLoudness(float max, float avg) {
    if (avg <= 0) return 0;
    float loudnessRatio = max / avg;
    return mapFloat(loudnessRatio, 0.6f, 2.8f, 1, visemeFramelength);
}

unsigned int Viseme::smoothedLoudness(unsigned int input) {
    unsigned long currentTime = millis();
    unsigned long decayElapsedTime = currentTime - decayStartTime;

    if (input >= currentLoudness) {
        // Increment limit
        if (input - currentLoudness > 5) {
            input = currentLoudness + 5;
            input = input > visemeFramelength ? visemeFramelength : input;
        }
        currentLoudness = input;
        decayStartTime = 0;
    } else {
        if (decayStartTime == 0) {
            decayStartTime = currentTime;
        } else {
            if (decayElapsedTime >= decayElapsedThreshold && currentLoudness <= 5) {
                currentLoudness = 0;
                decayStartTime = 0;
            } else {
                unsigned int decayedInput = static_cast<unsigned int>(currentLoudness - (decayRate * decayElapsedTime));
                if (decayedInput < input) {
                    currentLoudness = input;
                    decayStartTime = 0;
                } else {
                    currentLoudness = decayedInput;
                }
            }
        }
    }
    return currentLoudness;
}

/**
 * Determine the dominant viseme based on highest amplitude.
 * Requires the dominant viseme to be clearly separated from second place.
 */
Viseme::VisemeType Viseme::getDominantViseme(float& maxAmp) {
    // Find max and second max
    float amps[5] = {ahAmplitude, eeAmplitude, ohAmplitude, ooAmplitude, thAmplitude};
    VisemeType names[5] = {AH, EE, OH, OO, TH};

    int maxIdx = 0;
    maxAmp = amps[0];
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

    // Require at least 5% separation for clear detection
    const float MIN_SEPARATION = 1.05f;
    if (maxAmp < noiseThreshold || maxAmp < secondMax * MIN_SEPARATION) {
        // No clear dominant viseme - return previous or default
        return previousViseme;
    }

    return names[maxIdx];
}

Viseme::VisemeType Viseme::holdViseme(VisemeType input) {
    VisemeType held_viseme = previousViseme;
    if (input != 0) {
        held_viseme = input;
    }
    previousViseme = held_viseme;
    return held_viseme;
}

const uint8_t* Viseme::visemeOutput(VisemeType viseme, unsigned int level) {
    static int previousLevel = -1;  // Initialize with an invalid value
    if (level == 0) {
        previousLevel = level;
        return mouthDefault;
    }
    if (level < previousLevel) {  // Hold viseme when level decreasing
        viseme = previousViseme;
    } else {
        previousViseme = viseme;
    }
    previousLevel = level;
    level -= 1;
    // Serial.print("Base:");
    // Serial.print(4);
    // Serial.print(",Level:");
    // Serial.print(level);
    // Serial.print(",viseme:");
    // Serial.print(viseme);
    // Serial.print(",previousViseme:");
    // Serial.println(previousViseme);
    // auto combination = visemeCombination.find(std::make_pair(viseme, previousViseme));
    // Serial.print(",combination:");
    // Serial.println(combination->second);
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

const uint8_t* Viseme::renderViseme() {
    // Audio processing pipeline using ESP-DSP
    readAndPrepareSamples();
    performFFT();
    calculateMagnitudes();
    analyzeVisemes();

    // Calculate statistics
    float min_amplitude, max_amplitude, avg_amplitude;
    calculateAmplitude(ahAmplitude, eeAmplitude, ohAmplitude, ooAmplitude, thAmplitude,
                      min_amplitude, max_amplitude, avg_amplitude);

    // Find dominant viseme
    float dominantMaxAmp;
    VisemeType dominantViseme = getDominantViseme(dominantMaxAmp);

    // Apply level boost
    levelBoost(dominantViseme, max_amplitude);

    // Calculate loudness level
    unsigned int loudness_level = calculateLoudness(max_amplitude, avg_amplitude);
    loudness_level = max_amplitude > noiseThreshold ? loudness_level : 0;
    loudness_level = smoothedLoudness(loudness_level);

    // Debugging output
    int max_threshold = 2000;
    Serial.print("Max_THRESHOLD:");
    Serial.print(max_threshold);
    Serial.print(",noiseThreshold:");
    Serial.print(noiseThreshold);
    Serial.print(",AH:");
    Serial.print(ahAmplitude > max_threshold ? max_threshold : ahAmplitude);
    Serial.print(",EE:");
    Serial.print(eeAmplitude > max_threshold ? max_threshold : eeAmplitude);
    Serial.print(",OH:");
    Serial.print(ohAmplitude > max_threshold ? max_threshold : ohAmplitude);
    Serial.print(",OO:");
    Serial.print(ooAmplitude > max_threshold ? max_threshold : ooAmplitude);
    Serial.print(",TH:");
    Serial.println(thAmplitude > max_threshold ? max_threshold : thAmplitude);
    Serial.print(",Level:");
    Serial.print(loudness_level * 100);
    Serial.print(",Max:");
    Serial.print(max_amplitude > max_threshold ? max_threshold : max_amplitude);
    Serial.print(",AVG_AMP:");
    Serial.println(avg_amplitude > max_threshold ? max_threshold : avg_amplitude);

    // Final render
    return visemeOutput(dominantViseme, loudness_level);
}