# KMMX-Fursuit BLE Interface Specification

## Overview
This document defines the Bluetooth Low Energy (BLE) GATT profile for controlling the KMMX-Fursuit system. Client applications should implement this specification to communicate with the fursuit controller.

**Device Name:** `KMMX-Fursuit`
**Manufacturer ID:** `0xFFFF` (Custom)
**Manufacturer Data:** `KMMX` + Version `1.0`

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

## Color Behavior
- **Display Effect Color 1**: Sets gradient top color, dual spiral color, and dual circle color
- **Display Effect Color 2**: Sets gradient bottom color only; Color 1 is preserved for dual patterns
