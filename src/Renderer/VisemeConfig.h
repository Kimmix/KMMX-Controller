#pragma once

#include <stdint.h>

// ============================================================================
// Viseme Detection Configuration
// ============================================================================
// These constants configure the audio-to-viseme (lip-sync) system that uses
// FFT analysis of microphone input to detect speech phonemes and animate
// the mouth accordingly.
//
// Frequency ranges are based on typical formant frequencies for each phoneme.
// Adjust these values to tune the detection sensitivity and accuracy.
// ============================================================================

// I2S Audio Configuration
const float i2sSampleRate = 8000.0f;                            // Sample rate in Hz (8kHz is sufficient for speech)
const uint16_t i2sSamples = 256;                                // Number of samples per FFT window (power of 2)

// Viseme Frequency Ranges (Hz)
// Each phoneme has a characteristic frequency range based on vocal formants
const uint16_t visemeAhFreqMin = 700;                           // Start frequency of AH viseme (open mouth sound like "cat")
const uint16_t visemeAhFreqMax = 1500;                          // End frequency of AH viseme

const uint16_t visemeEeFreqMin = 1000;                          // Start frequency of EE viseme (teeth-showing sound like "see")
const uint16_t visemeEeFreqMax = 3000;                          // End frequency of EE viseme

const uint16_t visemeOhFreqMin = 400;                           // Start frequency of OH viseme (rounded mouth sound like "go")
const uint16_t visemeOhFreqMax = 1100;                          // End frequency of OH viseme

const uint16_t visemeOoFreqMin = 250;                           // Start frequency of OO viseme (pursed lips sound like "food")
const uint16_t visemeOoFreqMax = 900;                           // End frequency of OO viseme

const uint16_t visemeThFreqMin = 2800;                          // Start frequency of TH viseme (fricative sound like "think")
const uint16_t visemeThFreqMax = 4000;                          // End frequency of TH viseme

// Normalization Scale Factors
// These multipliers balance the sensitivity across different visemes
// Lower values = less sensitive, Higher values = more sensitive
constexpr float visemeAhScale = 3.5f;                               // AH viseme scale factor
constexpr float visemeEeScale = 1.3f;                               // EE viseme scale factor
constexpr float visemeOhScale = 1.5f;                               // OH viseme scale factor
constexpr float visemeOoScale = 1.1f;                               // OO viseme scale factor
constexpr float visemeThScale = 1.0f;                               // TH viseme scale factor
