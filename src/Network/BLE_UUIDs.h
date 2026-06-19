#pragma once

// ============================================================================
// BLE Service and Characteristic UUIDs
// ============================================================================
// These UUIDs define the BLE GATT service and characteristics for remote
// control of the KMMX fursuit controller via Bluetooth Low Energy.
//
// Each UUID is a 128-bit identifier that uniquely identifies a service or
// characteristic in the GATT profile. These are custom UUIDs generated for
// this project and should not be changed unless you also update the client
// application.
// ============================================================================

// Main GATT Service UUID
#define BLE_SERVICE_UUID "c1449275-bf34-40ab-979d-e34a1fdbb129"

// Display Control Characteristics
#define BLE_DISPLAY_BRIGHTNESS_CHARACTERISTIC_UUID "9fdfd124-966b-44f7-8331-778c4d1512fc"
#define BLE_DISPLAY_COLOR_MODE_CHARACTERISTIC_UUID "f5a6b7c8-d9e0-4f5a-b0c1-2d3e4f5a6b7c"
#define BLE_DISPLAY_EFFECT_COLOR1_CHARACTERISTIC_UUID "a6b7c8d9-e0f1-4a5b-c1d2-3e4f5a6b7c8d"
#define BLE_DISPLAY_EFFECT_COLOR2_CHARACTERISTIC_UUID "b7c8d9e0-f1a2-4b5c-d2e3-4f5a6b7c8d9e"
#define BLE_DISPLAY_EFFECT_OPTION1_CHARACTERISTIC_UUID "c7d8e9f0-a1b2-4c5d-e2f3-4a5b6c7d8e9f"
#define BLE_DISPLAY_EFFECT_OPTION2_CHARACTERISTIC_UUID "e7f8a9b0-c1d2-4e5f-a2b3-4c5d6e7f8a9b"
#define BLE_DISPLAY_EFFECT_OPTION3_CHARACTERISTIC_UUID "f7a8b9c0-d1e2-4f5a-b2c3-4d5e6f7a8b9c"

// Facial Expression Characteristics
#define BLE_EYE_STATE_CHARACTERISTIC_UUID "49a36bb2-1c66-4e5c-8ff3-28e55a64beb3"
#define BLE_MOUTH_STATE_CHARACTERISTIC_UUID "f6a7b8c9-d0e1-4f5a-b1c2-3d4e5f6a7b8c"
#define BLE_VISEME_CHARACTERISTIC_UUID "493d06f3-0fa0-4a90-88f1-ebaed0da9b80"

// Viseme Advanced Parameters
#define BLE_VISEME_ENVELOPE_ATTACK_UUID "d1e2f3a4-b5c6-47d8-9e0f-1a2b3c4d5e6f"
#define BLE_VISEME_ENVELOPE_RELEASE_UUID "d2e3f4a5-b6c7-48d9-9f0a-1b2c3d4e5f6a"
#define BLE_VISEME_ATTACK_THRESHOLD_UUID "d3e4f5a6-b7c8-49da-a0b1-2c3d4e5f6a7b"
#define BLE_VISEME_MIN_SEPARATION_UUID "d4e5f6a7-b8c9-4adb-a1b2-3d4e5f6a7b8c"
#define BLE_VISEME_NOISE_FLOOR_MIN_UUID "d6e7f8a9-bacb-4cdd-a3b4-5f6a7b8c9d0e"
#define BLE_VISEME_NOISE_FLOOR_MAX_UUID "d7e8f9aa-bbcc-4dde-a4b5-6a7b8c9d0e1f"
#define BLE_VISEME_NOISE_ADAPT_SPEED_UUID "d8e9faab-bccd-4edf-a5b6-7b8c9d0e1f2a"

// Viseme Scale Factors
#define BLE_VISEME_AH_SCALE_UUID "d9eafbac-bdce-4fe0-a6b7-8c9d0e1f2a3b"
#define BLE_VISEME_EE_SCALE_UUID "dafbfcad-becf-4ae1-a7b8-9d0e1f2a3b4c"
#define BLE_VISEME_OH_SCALE_UUID "dbfcfdae-bfd0-4be2-a8b9-0e1f2a3b4c5d"
#define BLE_VISEME_OO_SCALE_UUID "dcfdfebf-c0d1-4ce3-a9ba-1f2a3b4c5d6e"
#define BLE_VISEME_TH_SCALE_UUID "ddfeafc0-c1d2-4de4-aabb-2a3b4c5d6e7f"

// LED Brightness Characteristics
#define BLE_HORN_BRIGHTNESS_CHARACTERISTIC_UUID "a1b2c3d4-e5f6-4a5b-8c9d-0e1f2a3b4c5d"
#define BLE_CHEEK_BRIGHTNESS_CHARACTERISTIC_UUID "b2c3d4e5-f6a7-4b5c-9d0e-1f2a3b4c5d6e"
#define BLE_CHEEK_BG_COLOR_CHARACTERISTIC_UUID "c3d4e5f6-a7b8-4c5d-9e0f-1a2b3c4d5e6f"
#define BLE_CHEEK_FADE_COLOR_CHARACTERISTIC_UUID "d4e5f6a7-b8c9-4d5e-9f0a-1b2c3d4e5f6a"

// System Control Characteristics
#define BLE_REBOOT_CHARACTERISTIC_UUID "e5f6a7b8-c9d0-4e5f-a0b1-2c3d4e5f6a7b"

// Motion Detection & Glitch Control Characteristics
#define BLE_GLITCH_TRIGGER_CHARACTERISTIC_UUID "a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f1"
#define BLE_MOTION_ENABLE_FLAGS_CHARACTERISTIC_UUID "a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f2"
#define BLE_TAP_SENSITIVITY_CHARACTERISTIC_UUID "a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f3"
#define BLE_GLITCH_INTENSITY_CHARACTERISTIC_UUID "a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f4"

// Fan Control Characteristics
#define BLE_FAN_SPEED_CHARACTERISTIC_UUID "f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f1"
#define BLE_FAN_ENABLED_CHARACTERISTIC_UUID "f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f2"
#define BLE_FAN_RPM_CHARACTERISTIC_UUID "f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f3"
#define BLE_FAN_CONNECTED_CHARACTERISTIC_UUID "f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f4"
