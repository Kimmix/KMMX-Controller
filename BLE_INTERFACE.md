# KMMX-Fursuit BLE Interface Specification

## Overview
This document defines the Bluetooth Low Energy (BLE) GATT profile for controlling the KMMX-Fursuit system. Client applications should implement this specification to communicate with the fursuit controller.

**Device Name:** `KMMX-Fursuit`
**Manufacturer ID:** `0xFFFF` (Custom)
**Manufacturer Data:** `KMMX` + Version `1.0`

## Hardware Versions

The KMMX controller supports two hardware versions with different capabilities:

| Feature | V2 | V4 |
|---------|----|----|
| Accelerometer | LIS3DH (0x18) | MPU6050 (0x68) |
| Status LED | WS2812 RGB (IO45) | SK6812 RGB (IO38) |
| Fan Control | ❌ Not available | ✅ Available |
| PSRAM | None | 8MB |

**Note:** Fan control characteristics (see below) are only available on **V4 hardware**. Clients should gracefully handle these characteristics being absent on V2 devices.

---

## GATT Service

**Service UUID:** `c1449275-bf34-40ab-979d-e34a1fdbb129`

---

## Characteristics

### Display Control

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Display Brightness | `9fdfd124-966b-44f7-8331-778c4d1512fc` | READ, WRITE | `uint8_t` (0-100) | Controls overall display brightness. 0 = off, 100 = maximum brightness. |
| Display Color Mode | `f5a6b7c8-d9e0-4f5a-b0c1-2d3e4f5a6b7c` | READ, WRITE | `uint8_t` | Selects display color mode/pattern. Mode-specific values. |
| Display Effect Color 1 | `a6b7c8d9-e0f1-4a5b-c1d2-3e4f5a6b7c8d` | READ, WRITE | `uint8_t[3]` (RGB) | Primary effect color for gradients and dual patterns. Format: [R, G, B] (0-255 each). |
| Display Effect Color 2 | `b7c8d9e0-f1a2-4b5c-d2e3-4f5a6b7c8d9e` | READ, WRITE | `uint8_t[3]` (RGB) | Secondary effect color for gradients. Format: [R, G, B] (0-255 each). |
| Display Effect Option 1 | `c7d8e9f0-a1b2-4c5d-e2f3-4a5b6c7d8e9f` | READ, WRITE | `uint8_t` | Effect thickness parameter. Range depends on active effect. |
| Display Effect Option 2 | `e7f8a9b0-c1d2-4e5f-a2b3-4c5d6e7f8a9b` | READ, WRITE | `uint8_t` | Effect animation speed. Higher values = faster animation. |
| Display Effect Option 3 | `f7a8b9c0-d1e2-4f5a-b2c3-4d5e6f7a8b9c` | READ, WRITE | `uint8_t` (0/1) | Effect direction inversion. 0 = normal, 1 = inverted. |

### Facial Expressions

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Eye State | `49a36bb2-1c66-4e5c-8ff3-28e55a64beb3` | READ, WRITE | `uint8_t` | Changes eye expression state. See expression mapping below. |
| Mouth State | `f6a7b8c9-d0e1-4f5a-b1c2-3d4e5f6a7b8c` | READ, WRITE | `uint8_t` | Changes mouth expression state. See expression mapping below. |
| Viseme | `493d06f3-0fa0-4a90-88f1-ebaed0da9b80` | READ, WRITE | `uint8_t` (0-10) | Controls automatic viseme/lip-sync system. See values below. |

**Viseme Control Values:**
- **0**: Disable viseme (mouth returns to IDLE)
- **1**: Enable viseme (mouth enters TALKING mode with automatic lip-sync)
- **2-10**: Enable viseme and set noise threshold sensitivity (2 = lowest/400, 10 = highest/25000)

*Note: When enabled, the system automatically analyzes microphone input and generates mouth shapes (AH, EE, OH, OO, TH) based on frequency analysis.*

### Viseme Advanced Parameters

**Note:** These characteristics provide fine-grained control over the viseme detection algorithm. Values are sent as little-endian binary (float or uint16_t).

| Name | UUID | Properties | Data Format | Range | Default | Description |
|------|------|------------|-------------|-------|---------|-------------|
| Envelope Attack | `d1e2f3a4-b5c6-47d8-9e0f-1a2b3c4d5e6f` | READ, WRITE | `float` | 0.1-0.9 | 0.3 | Attack time constant (higher = faster rise) |
| Envelope Release | `d2e3f4a5-b6c7-48d9-9f0a-1b2c3d4e5f6a` | READ, WRITE | `float` | 0.01-0.5 | 0.1 | Release time constant (lower = slower decay) |
| Attack Threshold | `d3e4f5a6-b7c8-49da-a0b1-2c3d4e5f6a7b` | READ, WRITE | `float` | 1.0-3.0 | 1.25 | Envelope ratio to detect syllable start (1.25 = 25% increase) |
| Min Separation | `d4e5f6a7-b8c9-4adb-a1b2-3d4e5f6a7b8c` | READ, WRITE | `float` | 1.0-2.0 | 1.10 | Required separation between 1st and 2nd viseme (1.10 = 10% stronger) |
| Noise Floor Min | `d6e7f8a9-bacb-4cdd-a3b4-5f6a7b8c9d0e` | READ, WRITE | `float` | 1.0-50.0 | 5.0 | Minimum adaptive noise floor threshold (RMS scale) |
| Noise Floor Max | `d7e8f9aa-bbcc-4dde-a4b5-6a7b8c9d0e1f` | READ, WRITE | `float` | 5.0-200.0 | 50.0 | Maximum adaptive noise floor threshold (RMS scale) |
| Noise Adapt Speed | `d8e9faab-bccd-4edf-a5b6-7b8c9d0e1f2a` | READ, WRITE | `float` | 0.0001-0.01 | 0.001 | Speed of noise floor adaptation |
| AH Scale | `d9eafbac-bdce-4fe0-a6b7-8c9d0e1f2a3b` | READ, WRITE | `float` | 0.1-5.0 | 0.4 | Sensitivity multiplier for AH viseme |
| EE Scale | `dafbfcad-becf-4ae1-a7b8-9d0e1f2a3b4c` | READ, WRITE | `float` | 0.1-5.0 | 0.3 | Sensitivity multiplier for EE viseme |
| OH Scale | `dbfcfdae-bfd0-4be2-a8b9-0e1f2a3b4c5d` | READ, WRITE | `float` | 0.1-5.0 | 1.5 | Sensitivity multiplier for OH viseme |
| OO Scale | `dcfdfebf-c0d1-4ce3-a9ba-1f2a3b4c5d6e` | READ, WRITE | `float` | 0.1-5.0 | 1.4 | Sensitivity multiplier for OO viseme |
| TH Scale | `ddfeafc0-c1d2-4de4-aabb-2a3b4c5d6e7f` | READ, WRITE | `float` | 0.1-5.0 | 3.0 | Sensitivity multiplier for TH viseme |

**Tuning Tips:**
- **Envelope Attack/Release:** Controls how quickly the loudness tracker responds to sound
- **Attack Threshold:** Lower values detect more syllables, higher values detect only clear syllables
- **Min Separation:** Increase to reduce flickering between visemes
- **Noise Floor:** System auto-adjusts between min/max based on ambient noise
- **Viseme Scales:** Adjust individual viseme sensitivity if certain mouth shapes are under/over-detected

### LED Brightness Control

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Horn Brightness | `a1b2c3d4-e5f6-4a5b-8c9d-0e1f2a3b4c5d` | READ, WRITE | `uint8_t` (0-100) | Controls horn LED brightness. 0 = off, 100 = maximum. |
| Cheek Brightness | `b2c3d4e5-f6a7-4b5c-9d0e-1f2a3b4c5d6e` | READ, WRITE | `uint8_t` (0-100) | Controls cheek LED brightness. 0 = off, 100 = maximum. |
| Cheek Background Color | `c3d4e5f6-a7b8-4c5d-9e0f-1a2b3c4d5e6f` | READ, WRITE | `uint8_t[3]` (RGB) | Sets cheek background color. Format: [R, G, B] (0-255 each). |
| Cheek Fade Color | `d4e5f6a7-b8c9-4d5e-9f0a-1b2c3d4e5f6a` | READ, WRITE | `uint8_t[3]` (RGB) | Sets cheek fade/transition color. Format: [R, G, B] (0-255 each). |

### System Control

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Reboot | `e5f6a7b8-c9d0-4e5f-a0b1-2c3d4e5f6a7b` | WRITE | `uint8_t` (0/1) | Triggers system reboot. Write non-zero value to reboot. |

### Motion Detection & Glitch Control

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Glitch Trigger | `a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f1` | WRITE | `uint8_t` (0-100) | Manually triggers a glitch effect with specified intensity. 0 = subtle, 100 = maximum intensity. |
| Motion Enable Flags | `a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f2` | READ, WRITE | `uint8_t` (bitfield) | Enable/disable motion detection features. Bit 0: Tap Detection, Bit 1: Petting Detection, Bit 2: Tilt Detection, Bit 3: Upside Down Detection, Bit 4: Boop Toggle. |
| Tap Sensitivity | `a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f3` | READ, WRITE | `uint8_t` (0-100) | Adjusts tap detection sensitivity. 0 = very sensitive (light taps), 100 = requires hard taps. |
| Glitch Intensity | `a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f4` | READ, WRITE | `uint8_t` (0-100) | Controls automatic glitch effect intensity range. 0 = subtle glitches, 100 = dramatic glitches. Affects duration, row count, and visual intensity. |

**Motion Enable Flags Bitfield:**
- **Bit 0 (0x01)**: Enable Tap Detection (triggers glitch on tap)
- **Bit 1 (0x02)**: Enable Petting Detection (triggers smile on repeated pats)
- **Bit 2 (0x04)**: Enable Tilt Detection (triggers expressions on head tilt)
- **Bit 3 (0x08)**: Enable Upside Down Detection (triggers crying when held upside down)
- **Bit 4 (0x10)**: Enable Boop Toggle (triggers boop interaction)

**Example:** `0x09` (binary: 1001) = Tap Detection + Upside Down Detection enabled

---

### Fan Control (V4 Only)

**⚠️ V4 Hardware Only:** These characteristics are only available on KimmixControllerV4 boards. V2 boards do not have fan control hardware.

| Name | UUID | Properties | Data Format | Description |
|------|------|------------|-------------|-------------|
| Fan Speed | `f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f1` | READ, WRITE | `uint8_t` (0-100) | Controls fan speed. 0 = off, 100 = maximum speed. Smooth transitions are applied automatically. |
| Fan Enabled | `f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f2` | READ, WRITE | `uint8_t` (0/1) | Enable/disable fan control. 0 = disabled (fan forced off), 1 = enabled. |
| Fan RPM | `f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f3` | READ, NOTIFY | `uint16_t` (0-65535) | Current fan speed in RPM (revolutions per minute). Read-only tachometer feedback. Subscribe to NOTIFY for real-time updates. |
| Fan Connected | `f1f2f3f4-a1a2-4b1b-c1c2-d1d2d3d4d5f4` | READ, NOTIFY | `uint8_t` (0/1) | Fan connection status. 0 = disconnected/not responding, 1 = connected and responding. Subscribe to NOTIFY for connection changes. |

**Fan Behavior Notes:**
- **Graceful Degradation**: Controller operates normally without fan connected.
- **Connection Detection**: Monitors tachometer when speed > 0. Fan marked disconnected if RPM < 100 for 3+ seconds.
- **Safe Defaults**: Fan disabled (speed = 0) on boot.
- **Smooth Transitions**: Speed changes use automatic fade transitions.
- **PWM Control**: 4-pin PWM (25kHz, 8-bit) via TXU0202DCUR level shifter.
- **Update Rate**: RPM updates every 1 second, connection check every 3 seconds.

---

## Color Behavior
- **Display Effect Color 1**: Sets gradient top color, dual spiral color, and dual circle color
- **Display Effect Color 2**: Sets gradient bottom color only; Color 1 is preserved for dual patterns

---

## Characteristic Summary by Hardware Version

### Available on Both V2 and V4
✅ Display Brightness
✅ Display Color Mode
✅ Display Effect Colors (1, 2)
✅ Display Effect Options (1, 2, 3)
✅ Eye State
✅ Mouth State
✅ Viseme
✅ Horn Brightness
✅ Cheek Brightness
✅ Cheek Background Color
✅ Cheek Fade Color
✅ Reboot
✅ Glitch Trigger
✅ Motion Enable Flags
✅ Tap Sensitivity
✅ Glitch Intensity

### V4 Only
🆕 Fan Speed
🆕 Fan Enabled
🆕 Fan RPM
🆕 Fan Connected

**Total Characteristics:**
- V2: 19 characteristics
- V4: 23 characteristics (19 + 4 fan control)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Initial | Original KMMX-Fursuit BLE specification (V2 hardware) |
| 2.0 | 2026-06 | Added V4 hardware support, fan control characteristics, hardware version documentation |
