#pragma once

#include <stdint.h>

#ifdef DISABLE_SERIAL_LOGGING
const bool enableMotionDebug = false;
#else
const bool enableMotionDebug = true;
#endif

// ============================================================================
// MPU6050 Gyroscope Calibration (MPU6050 only)
// ============================================================================
// Enable/disable automatic gyroscope calibration on startup
// When enabled, the MPU6050 will calibrate gyroscope offsets during setUp()
// This takes ~4 seconds and requires the device to be stationary
inline bool enableGyroCalibration = false;

// Tilt Detection
inline bool enableTiltDetection = false;
inline float tiltThreshold = 4.0f;
inline float tiltNeutralThreshold = 3.0f;
constexpr uint16_t tiltSustainTime = 500;
constexpr uint16_t tiltDebounceTime = 300;
constexpr uint16_t tiltDirectionChangeCooldown = 3000;

// Upside Down Detection
inline bool enableUpsideDownDetection = true;
inline float upsideDownThreshold = -7.0f;
constexpr uint16_t upsideDownSustainTime = 1500;
constexpr uint16_t upsideDownDebounceTime = 500;

// Petting Detection
inline bool enablePettingDetection = true;
constexpr bool enablePettingCooldownAfterTilt = true;
constexpr uint16_t pettingCooldownAfterTilt = 1000;
inline float pettingSpikeThreshold = 0.8f;
constexpr uint16_t pettingSpikeCooldown = 300;
constexpr float pettingHappinessPerPat = 20.0f;
constexpr float pettingHappinessTrigger = 80.0f;
constexpr float pettingHappinessDecayRate = 15.0f;
constexpr float pettingHappinessEndThreshold = 20.0f;
constexpr float pettingDeltaTimeMax = 0.5f;

// Boop Detection
inline bool enableBoopDetection = true;

// Tap Detection
inline bool enableTapDetection = true;
inline float tapSpikeThreshold = 1.0f;
constexpr uint16_t tapCooldown = 100;
constexpr uint16_t tapPeakDecayWindow = 50;
constexpr float tapDecayRatio = 0.4f;
constexpr uint16_t tapGlitchMinDuration = 300;
constexpr uint16_t tapGlitchMaxDuration = 1200;
inline int tapGlitchMinIntensity = 10;
inline int tapGlitchMaxIntensity = 60;
constexpr float tapMagnitudeMin = 1.0f;
constexpr float tapMagnitudeMax = 10.0f;
constexpr uint16_t tapGlitchUpdateInterval = 75;
constexpr int tapGlitchFullScreenChance = 15;
constexpr uint16_t tapGlitchRampDuration = 50;
constexpr uint8_t tapGlitchMinRows = 1;
constexpr uint8_t tapGlitchMaxRows = 8;

// Rotation-Based Gesture Detection (MPU6050 only)
inline bool enableShakeDetection = true;
inline float shakeThreshold = 4.0f;
constexpr uint16_t shakeSustainTime = 300;
constexpr uint16_t shakeCooldown = 2000;

inline bool enableSpinDetection = true;
inline float spinThreshold = 3.0f;
constexpr uint16_t spinSustainTime = 500;
constexpr uint16_t spinCooldown = 3000;

inline bool enableNodDetection = true;
inline float nodThreshold = 2.5f;
constexpr uint16_t nodSustainTime = 200;
constexpr uint16_t nodCooldown = 1500;

// Idle Detection
const uint32_t idleTimeout = 10 * 60000;
const uint16_t baselineUpdateDelay = 2000;
const float idleAccThreshold = 1.5f;
const float sleepingAccThreshold = 0.7f;
const uint8_t motionHysteresisCount = 3;
const uint8_t motionCounterDecayRate = 10;
const float gravityMagnitude = 9.81f;
