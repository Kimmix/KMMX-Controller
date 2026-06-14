#pragma once

// ============================================================================
// KimmixControllerV4 Pin Configuration
// ============================================================================
// Board: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
// Accelerometer: MPU6050 (0x68)
// Status LED: SK6812 RGB (IO38)
// Features: Fan Control Enabled
// ============================================================================

#define BOARD_NAME "KimmixControllerV4"
#define PIN_CONFIG_VERSION 4

// --- I2C Pins ---
#define S3_SDA 21
#define S3_SCL 47

// --- I2S Audio Pins ---
#define I2S_WS 11                   // Word Select (LRCLK)
#define I2S_SD 13                   // Serial Data (DATA)
#define I2S_SCK 12                  // Serial Clock (BCLK/CLK)
#define I2S_PORT I2S_NUM_0          // I2S peripheral number

// --- HUB75 LED Matrix Pins ---
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
#define CLK 14
#define LAT 3
#define OE 9

// --- Other Pins ---
#define LED_PWM_PIN 42              // Horn LED (PWM controlled)
#define RGB_STATUS_PIN 38           // RGB status LED (SK6812)
#define ARGB_PIN 10                 // Side ARGB LED strip

// --- Fan Control Pins (V4 only) ---
#define HAS_FAN_CONTROL 1           // Feature flag for fan control
#define FAN_PWM_PIN 40              // Fan PWM control
#define FAN_SPD_PIN 41              // Fan tachometer (speed readback)
