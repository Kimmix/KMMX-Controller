#pragma once

// ============================================================================
// KimmixControllerV2 - Pin Configuration and Capabilities
// ============================================================================
// Board: ESP32-S3-N16R8 (16MB Flash, No PSRAM)
// Accelerometer: LIS3DH (I2C 0x18)
// Status LED: WS2812 RGB (IO45)
// ============================================================================

#define BOARD_NAME "KimmixControllerV2"

// ============================================================================
// Hardware Capability Flags
// ============================================================================

// Memory capabilities
#define HAS_PSRAM 0
#define PSRAM_SIZE_MB 0

// Feature capabilities
#define HAS_FAN_CONTROL 0
#define HAS_EXTERNAL_ANTENNA 0

// Sensor capabilities
#define ACCEL_TYPE_LIS3DH 1
#define ACCEL_TYPE_MPU6050 0

// LED capabilities
#define STATUS_LED_TYPE_WS2812 1
#define STATUS_LED_TYPE_SK6812 0

// ============================================================================
// Pin Definitions
// ============================================================================

// --- I2C Pins ---
#define S3_SDA 9
#define S3_SCL 3

// --- I2S Audio Pins ---
#define I2S_WS 10                   // Word Select (LRCLK)
#define I2S_SD 12                   // Serial Data
#define I2S_SCK 11                  // Serial Clock (BCLK)
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
#define CLK 41
#define LAT 40
#define OE 39

// --- Other Pins ---
#define LED_PWM_PIN 21              // Horn LED (PWM controlled)
#define RGB_STATUS_PIN 45           // RGB status LED (WS2812)
#define ARGB_PIN 14                 // Side ARGB LED strip (WS2812)
