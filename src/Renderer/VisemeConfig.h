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
const float i2sSampleRate = 16000.0f;                            // Sample rate in Hz (16 kHz speech capture)
const uint16_t i2sSamples = 512;                                // Number of samples per FFT window (power of 2)

// Normalization Scale Factors
// Formant scores (LipSync::formantScores) are already level-independent, so these start
// neutral; BLE can still bias a viseme. Lower = less sensitive, higher = more sensitive.
constexpr float visemeAhScale = 1.0f;                              // AH viseme scale factor
constexpr float visemeEeScale = 1.0f;                              // EE viseme scale factor
constexpr float visemeOhScale = 1.0f;                              // OH viseme scale factor
constexpr float visemeOoScale = 1.0f;                              // OO viseme scale factor
constexpr float visemeThScale = 1.0f;                              // TH viseme scale factor

constexpr uint16_t i2sHopSamples = 256; // 16 ms, 50% overlap
