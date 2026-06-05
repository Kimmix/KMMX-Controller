#pragma once

#include <stdint.h>

// ============================================================================
// Motion Detection Configuration
// ============================================================================
// These constants configure various motion detection features that use the
// accelerometer to detect user interactions like tilting, petting, tapping,
// and upside-down orientation. Adjust these values to tune the sensitivity
// and behavior of each detection mode.
// ============================================================================

// Global Motion Detection Settings
#ifdef DISABLE_SERIAL_LOGGING
const bool enableMotionDebug = false;                           // Disabled in production builds
#else
const bool enableMotionDebug = true;                            // Enable detailed debug logging for all motion detection
#endif

// ============================================================================
// Tilt Detection - Detects sustained head tilt for curious/confused expressions
// ============================================================================
inline bool enableTiltDetection = false;                         // Enable/disable tilt detection feature (BLE controllable)
inline float tiltThreshold = 4.0f;                               // m/s² threshold for tilt detection (BLE controllable)
inline float tiltNeutralThreshold = 3.0f;                        // m/s² threshold to return to neutral (BLE controllable)
constexpr uint16_t tiltSustainTime = 500;                        // Time to hold tilt before triggering (ms)
constexpr uint16_t tiltDebounceTime = 300;                       // Cooldown between tilt changes (ms)
constexpr uint16_t tiltDirectionChangeCooldown = 3000;           // Cooldown when switching from left/right to forward/back (ms)

// ============================================================================
// Upside Down Detection - Detects when the character is held upside down
// ============================================================================
inline bool enableUpsideDownDetection = true;                    // Enable/disable upside-down detection feature (BLE controllable)
inline float upsideDownThreshold = -7.0f;                        // m/s² threshold for Y-axis (BLE controllable)
constexpr uint16_t upsideDownSustainTime = 1500;                 // Time to hold upside down before triggering (ms)
constexpr uint16_t upsideDownDebounceTime = 500;                 // Cooldown between state changes (ms)

// ============================================================================
// Petting Detection - Detects quick pats/taps (spike-based) for contentment
// ============================================================================
inline bool enablePettingDetection = true;                       // Enable/disable petting detection feature (BLE controllable)
constexpr bool enablePettingCooldownAfterTilt = true;            // Enable cooldown after tilt to prevent immediate petting
constexpr uint16_t pettingCooldownAfterTilt = 1000;              // Cooldown duration after tilt ends (ms)

inline float pettingSpikeThreshold = 0.8f;                       // m/s² threshold for detecting a gentle pat/tap spike (BLE controllable)
constexpr uint16_t pettingSpikeCooldown = 300;                   // Minimum time between individual spikes (ms) - prevents double-counting

// Dynamic happiness system parameters
constexpr float pettingHappinessPerPat = 20.0f;                  // Happiness added per pat (0-100 scale)
constexpr float pettingHappinessTrigger = 80.0f;                 // Happiness level to trigger SMILE response (0-100)
constexpr float pettingHappinessDecayRate = 15.0f;               // Happiness decay per second when not petting
constexpr float pettingHappinessEndThreshold = 20.0f;            // Happiness level below which response ends (allows natural fade-out)
constexpr float pettingDeltaTimeMax = 2.0f;                      // Maximum delta time in seconds to accept (sanity check for time jumps)

// ============================================================================
// Boop Detection - Detects proximity sensor interactions
// ============================================================================
inline bool enableBoopDetection = true;                          // Enable/disable boop detection feature (BLE controllable)

// ============================================================================
// Tap Detection - Detects light taps for glitch effects
// ============================================================================
inline bool enableTapDetection = true;                           // Enable/disable tap detection feature (BLE controllable)
inline float tapSpikeThreshold = 1.0f;                           // m/s² threshold for detecting a light tap (BLE controllable)
constexpr uint16_t tapCooldown = 100;                            // Minimum time between taps (ms)

// Peak + decay verification (improves tap vs. motion detection)
constexpr uint16_t tapPeakDecayWindow = 50;                      // Time window to verify decay after peak (ms)
constexpr float tapDecayRatio = 0.4f;                            // Decay ratio to confirm tap (0.4 = must drop to 40% of peak)

// Tap glitch effect scaling based on tap magnitude
constexpr uint16_t tapGlitchMinDuration = 300;                   // Minimum duration of glitch effect (ms) for light taps
constexpr uint16_t tapGlitchMaxDuration = 1200;                  // Maximum duration of glitch effect (ms) for hard taps
inline int tapGlitchMinIntensity = 10;                           // Minimum intensity of glitch effect (0-100) (BLE controllable)
inline int tapGlitchMaxIntensity = 60;                           // Maximum intensity of glitch effect (0-100) (BLE controllable)
constexpr float tapMagnitudeMin = 1.0f;                          // Minimum tap magnitude for scaling (m/s²)
constexpr float tapMagnitudeMax = 10.0f;                         // Maximum tap magnitude for scaling (m/s²)
constexpr uint16_t tapGlitchUpdateInterval = 75;                 // How often glitch pattern changes (ms)
constexpr int tapGlitchFullScreenChance = 15;                    // Chance (0-100) for full-screen glitch instead of localized
constexpr uint16_t tapGlitchRampDuration = 50;                   // Duration of smooth ramp-up at start (ms) - reduces jarring effect
constexpr uint8_t tapGlitchMinRows = 1;                          // Minimum glitch rows for low intensity (2-8 range)
constexpr uint8_t tapGlitchMaxRows = 8;                          // Maximum glitch rows for high intensity (2-8 range)

// ============================================================================
// Idle Detection Configuration
// ============================================================================
// Settings for detecting when the device is idle and putting it to sleep
// ============================================================================
const uint32_t idleTimeout = 300000;                            // Time till sleep in ms (300 seconds = 5 minutes)
const uint16_t baselineUpdateDelay = 2000;                      // Time to wait before updating baseline when still (2 seconds)
const float idleAccThreshold = 1.5f;                            // Magnitude change threshold to detect motion (m/s²)
const float sleepingAccThreshold = 0.7f;                        // Magnitude change threshold to wake up from sleep (m/s²)
const uint8_t motionHysteresisCount = 3;                        // Number of motion detections needed to wake up
const uint8_t motionCounterDecayRate = 10;                      // Samples between counter decay (10 = ~200ms at 50Hz)
const float gravityMagnitude = 9.81f;                           // Earth's gravity for reference (m/s²)
