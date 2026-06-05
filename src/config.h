#pragma once

#include <stdint.h>

// ============================================================================
// KMMX Fursuit Controller - Hardware Configuration
// ============================================================================
// This file contains the primary hardware configuration including pin
// assignments, display geometry, and user-adjustable preferences.
//
// Component-specific configurations have been moved to their respective
// module headers for better organization:
// - BLE UUIDs:              src/Network/BLE_UUIDs.h
// - Motion Detection:       src/KMMXController/MotionDetectionConfig.h
// - GooglyEye Physics:      src/Renderer/GooglyEyeConfig.h
// - Viseme/Audio:           src/Renderer/VisemeConfig.h
// ============================================================================

// ============================================================================
// SECTION 1: Hardware Pin Assignments (ESP32-S3)
// ============================================================================

// --- I2C Pins ---
#define S3_SDA 9
#define S3_SCL 3

// --- I2S Audio Pins ---
#define I2S_WS 10                   // Word Select (LRCLK)
#define I2S_SD 12                   // Serial Data
#define I2S_SCK 11                  // Serial Clock (BCLK)
#define I2S_PORT I2S_NUM_0          // I2S peripheral number

// --- HUB75 LED Matrix Pins (avoid QSPI pins) ---
#define R1 4
#define G1 5
#define BL1 6
#define R2 7
#define G2 15
#define BL2 16
#define CH_A 18
#define CH_B 8
#define CH_C 19
#define CH_D 20
#define CH_E 17                     // Required for two panels or 64x64 panels with 1/32 scan
#define CLK 41
#define LAT 40
#define OE 39

// --- Other Pins ---
#define LED_PWM_PIN 21              // Horn LED (PWM controlled)
#define RGB_STATUS_PIN 45           // RGB status LED (onboard)
#define ARGB_PIN 14                 // Side ARGB LED strip (WS2812)

// ============================================================================
// SECTION 2: BLE Device Names (can be overridden by platformio.ini)
// ============================================================================
// Note: BLE UUIDs have been moved to src/Network/BLE_UUIDs.h
// ============================================================================

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "KMMX"
#endif

#ifndef BLE_LOCAL_NAME
#define BLE_LOCAL_NAME "KMMX-BLE"
#endif

// ============================================================================
// SECTION 3: Display Geometry and Configuration
// ============================================================================

// --- HUB75 LED Matrix Configuration ---
const uint8_t panelResX = 64;                   // Single panel width in pixels
const uint8_t panelResY = 32;                   // Single panel height in pixels
const uint8_t panelsNumber = 2;                 // Number of panels chained together

// Calculated screen dimensions (automatically derived)
const uint8_t screenWidth = panelResX * panelsNumber;   // Total screen width = 128 pixels
const uint8_t screenHeight = panelResY;                 // Total screen height = 32 pixels

// Display settings
const uint8_t panelInitBrightness = 255;        // Initial brightness (0-255)
const uint16_t minRefreshRate = 240;            // Minimum refresh rate in Hz
const bool doubleBuffer = true;                 // Enable double buffering for smoother animations
const uint8_t minimumPixelBrightness = 40;      // Minimum pixel brightness threshold for drawing

// --- Facial Feature Geometry (position and size in pixels) ---
// Nose
const uint8_t noseWidth = 8;
const uint8_t noseHeight = 5;
const uint8_t noseOffsetX = 56;
const uint8_t noseOffsetY = 7;

// Eyes
const uint8_t eyeWidth = 32;
const uint8_t eyeHeight = 18;
const uint8_t eyeOffsetX = 15;
const uint8_t eyeOffsetY = 0;

// Mouth
const uint8_t mouthWidth = 50;
const uint8_t mouthHeight = 14;
const uint8_t mouthOffsetX = 14;
const uint8_t mouthOffsetY = 18;

// GooglyEye pupil position (depends on eyeOffsetX from above)
// Note: Physics constants are in src/Renderer/GooglyEyeConfig.h
const uint8_t googlyPupilOffsetX = eyeOffsetX + 8;

// ============================================================================
// SECTION 4: LED Configuration and User Preferences
// ============================================================================

// --- Side LED Strip (WS2812) ---
const uint8_t argbCount = 24;                   // Number of LEDs in the side strip
const uint8_t sideLEDBrightness = 128;          // LED brightness (0-255, mapped from 0-100 via BLE)
const uint32_t sideColor1RGB = 0xFF446C;        // Base color: #FF446C Reddish Pink
const uint32_t sideColor2RGB = 0xF9826C;        // Wave color: #F9826C Coral
const uint16_t sideLEDFadeInterval = 100;       // Wave update interval (ms) - slow, relaxed animation

// --- Horn LED (PWM) ---
const uint8_t hornBrightness = 20;              // Horn brightness (0-100)
const uint8_t hornPwmChannel = 0;               // PWM channel number
const uint16_t hornFrequency = 20000;           // PWM frequency in Hz
const uint8_t hornResolution = 8;               // PWM resolution in bits
const uint8_t hornMinBrightness = 2;            // Minimum safe brightness
const uint8_t hornMaxBrightness = 200;          // Maximum safe brightness (255 can cause overheating!)

// ============================================================================
// SECTION 5: Sensor and Interaction Configuration
// ============================================================================

// --- Sensor Update Rate ---
const uint8_t sensorUpdateInterval = 20;        // Sensor update interval in ms (50Hz)

// --- Proximity/Boop Detection ---
const uint16_t boopMinThreshold = 100;          // Minimum proximity distance to activate boop
const uint16_t boopMaxThreshold = 900;          // Maximum proximity detection range
const uint16_t boopMaxDuration = 2000;          // Maximum duration for calculating boop speed (ms)

// ============================================================================
// COMPONENT-SPECIFIC CONFIGURATIONS
// ============================================================================
// The following configurations have been moved to their respective module
// headers for better organization and maintainability:
//
// - Motion Detection (Tilt, Petting, Tap, Idle, Upside-Down):
//   → src/KMMXController/MotionDetectionConfig.h
//
// - GooglyEye Physics (acceleration, drag, elasticity):
//   → src/Renderer/GooglyEyeConfig.h
//
// - Viseme/Audio Processing (FFT, frequency ranges, I2S settings):
//   → src/Renderer/VisemeConfig.h
//
// - BLE Service and Characteristic UUIDs:
//   → src/Network/BLE_UUIDs.h
//
// Include the appropriate header files in your module implementations.
// ============================================================================