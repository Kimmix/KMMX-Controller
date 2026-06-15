#pragma once

// ============================================================================
// KimmixControllerV4 - Pin Configuration and Capabilities
// ============================================================================
// Board: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
// Accelerometer: MPU6050 (I2C 0x68)
// Status LED: SK6812 3535 RGB (IO38)
// Features: Fan Control, PSRAM
// ============================================================================

#define BOARD_NAME "KimmixControllerV4"

// ============================================================================
// Hardware Capability Flags
// ============================================================================

// Memory capabilities
#define HAS_PSRAM 1
#define PSRAM_SIZE_MB 8

// Feature capabilities
#define HAS_FAN_CONTROL 1
#define HAS_EXTERNAL_ANTENNA 0

// Sensor capabilities
#define ACCEL_TYPE_LIS3DH 0
#define ACCEL_TYPE_MPU6050 1

// LED capabilities
#define STATUS_LED_TYPE_WS2812 0
#define STATUS_LED_TYPE_SK6812 1

// ============================================================================
// Pin Definitions
// ============================================================================

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
#define RGB_STATUS_PIN 38           // RGB status LED (SK6812 3535 RGB - RGB color order)
#define ARGB_PIN 10                 // Side ARGB LED strip

// --- Fan Control Pins (V4 only) ---
#if HAS_FAN_CONTROL
#define FAN_PWM_PIN 40              // Fan PWM control
#define FAN_SPD_PIN 41              // Fan tachometer (speed readback)
#endif
