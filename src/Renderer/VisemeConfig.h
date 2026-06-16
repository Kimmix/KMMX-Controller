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

// Signal Processing Parameters
const float visemeNoiseThreshold = 400.0f;                      // Initial noise threshold for viseme to activate (lower = more sensitive)
const float visemeSmoothingAlpha = 0.10f;                       // Exponential smoothing factor for DC removal (0-1, lower = smoother but slower)

// Envelope Tracking Parameters
const float visemeEnvelopeAttack = 0.3f;                        // Attack time constant (0-1, higher = faster response to increases)
const float visemeEnvelopeRelease = 0.1f;                       // Release time constant (0-1, lower = slower decay)

// Adaptive Noise Floor Parameters
const float visemeNoiseFloorMin = 200.0f;                       // Minimum adaptive noise floor
const float visemeNoiseFloorMax = 1000.0f;                      // Maximum adaptive noise floor
const float visemeNoiseAdaptSpeed = 0.001f;                     // Speed of noise floor adaptation

// Attack Detection Parameters
const float visemeAttackThreshold = 1.25f;                      // Envelope increase ratio to detect attack
const uint16_t visemeMinAttackInterval = 50;                    // Minimum time between attacks (ms)

// Confidence & Switching Parameters
const float visemeMinSeparation = 1.10f;                        // Minimum separation ratio between 1st and 2nd viseme (1.10 = 10% stronger)
const uint16_t visemeMinHoldTime = 60;                          // Minimum time to hold a viseme before switching (ms)
const uint16_t visemeAttackTimeout = 120;                       // Timeout to allow switching without attack detection (ms)

// Normalization Scale Factors
// These multipliers balance the sensitivity across different visemes
// Lower values = less sensitive, Higher values = more sensitive
const float visemeAhScale = 1.0f;                               // AH viseme scale factor
const float visemeEeScale = 1.3f;                               // EE viseme scale factor
const float visemeOhScale = 1.5f;                               // OH viseme scale factor
const float visemeOoScale = 1.4f;                               // OO viseme scale factor
const float visemeThScale = 3.0f;                               // TH viseme scale factor

// Loudness Calculation Parameters
const float visemeLoudnessMinRatio = 0.8f;                      // Minimum ratio for loudness mapping (lower = more sensitive to quiet sounds)
const float visemeLoudnessMaxRatio = 5.0f;                      // Maximum ratio for loudness mapping (higher = less likely to max out)
const float visemeEnvelopeWeight = 0.4f;                        // Weight of envelope in loudness calculation (0.0-1.0, higher = more stable)
const float visemeFftWeight = 0.6f;                             // Weight of FFT ratio in loudness calculation (0.0-1.0, higher = more responsive)

// Debug Configuration
#define VISEME_DEBUG_PLOTTER 1
