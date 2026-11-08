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
| Viseme | `493d06f3-0fa0-4a90-88f1-ebaed0da9b80` | READ, WRITE | `uint8_t` (0-1) | Controls automatic viseme/lip-sync system. See values below. |

**Viseme Control Values:**
- **0**: Disable viseme (mouth returns to IDLE)
- **1**: Enable viseme (mouth enters TALKING mode with automatic lip-sync)

*Note: When enabled, the system automatically analyzes microphone input and generates mouth shapes (AH, EE, OH, OO, TH) based on frequency analysis.*

### Viseme tuning (settings v2, status v3)

The existing lip-sync on/off UUID is unchanged. The 13 old advanced viseme
characteristics are absent; their UUIDs are not repurposed. Discover the new
characteristics and check settings version 2 and status version 3 before enabling tuning. Unsupported/old firmware
keeps basic controls and shows **Update firmware to tune lip sync**. Old panels on
new firmware retain basic controls, but their cached advanced sliders are unsupported.

| Characteristic | UUID | Properties | Length |
|---|---|---|---|
| Viseme settings | `b9725d01-7546-4e0b-9b91-486e25012a02` | READ, WRITE | 10 bytes |
| Calibration command | `b9725d02-7546-4e0b-9b91-486e25012a02` | WRITE | 1 byte |
| Viseme status | `b9725d03-7546-4e0b-9b91-486e25012a02` | READ, NOTIFY | 16 bytes |

All multibyte fields are little-endian. Serialize fields explicitly, without C struct
padding. Each settings write replaces the complete record atomically.

| Settings offset | Type | Meaning / validation |
|---|---|---|
| 0 | uint8 | Version, exactly 2 |
| 1 | float32 | Fixed RMS opening threshold, 1–1,000; default 13 |
| 5 | float32 | Mouth gain, 0.25–4; default 1 |
| 9 | uint8 | Response: 0 Snappy, 1 Natural (default), 2 Smooth |

Reject wrong lengths, unsupported versions, NaN/infinity, invalid ranges/enums, and
settings writes during calibration. Rejection changes no settings. READ always returns
the canonical accepted values, including immediately after a rejected write. Inspect
status and compare readback; a successful transport write alone does not prove acceptance.
Default settings golden bytes: `02 00 00 50 41 00 00 80 3f 01`.

Calibration command: **1 start**, **2 cancel**; all other payloads are invalid.
Start while already calibrating reports Busy; start with an unhealthy mic reports
AudioError. Cancel is idempotent. Commands never change the active expression.

| Status offset | Type | Meaning |
|---|---|---|
| 0 | uint8 | Version 3 |
| 1 | uint16 | Accepted settings revision |
| 3 | uint16 | Revision applied by audio processing |
| 5 | float32 | Latest DC-removed, unwindowed microphone RMS |
| 9 | uint8 | Gate: 0 closed, 1 open |
| 10 | uint8 | Selected shape: 0 AH, 1 EE, 2 OH, 3 OO, 4 TH |
| 11 | uint8 | Rendered opening level, 0–60 |
| 12 | uint8 | Calibration: 0 Idle, 1 Countdown, 2 Measuring, 3 Success, 4 Failed, 5 Cancelled |
| 13 | uint8 | Progress 0–100 within current calibration phase |
| 14 | uint8 | Mic: 0 Ready, 1 ReadError (recoverable), 2 InitError (reboot required) |
| 15 | uint8 | Last result: 0 OK, 1 Invalid, 2 Busy, 3 AudioError, 4 Clipping, 5 ZeroAudio, 6 TooLoud |

Revisions are session-local counters, start at 1, and wrap skipping 0; compare for
equality, not numerical ordering. Matching accepted/applied revisions mean the
settings reached processing. Mic faults can prevent application.
The status version changed because saved revision, save state, and storage errors
were removed. Update both panel and firmware for tuning; mismatched versions retain
basic controls. Status and calibration payloads and all three UUIDs are unchanged. Settings use the original 10-byte v2 payload. Panels must check both versions
before tuning; basic lip-sync on/off remains compatible.

Status is sent at 10 Hz while connected, plus gate, calibration, revision, health, and
result transitions. READ is authoritative on reconnect; do not replay browser defaults.
The gate is independent of whether a temporary expression is showing; opening is zero
when lip-sync animation is inactive. A closed gate may have a nonzero closing-animation
level. Calibration reports a closed gate and zero opening.

**Signal behavior:** Open at RMS >= threshold. Close after 80 ms continuously below
80% of threshold. A threshold change re-evaluates on the next audio frame without the
old hold timer. Sustained speech never changes the threshold. While open, target
opening is `pow(clamp(max(RMS - threshold, 0) * gain / 80, 0, 1), 0.8)`; otherwise zero. At defaults the target starts above RMS 13 and reaches full
opening at RMS 93. Response attack/
release constants in milliseconds: Snappy 25/70, Natural 50/120, Smooth 90/200.
The 8 kHz, 256-sample analysis cadence is nominally 32 ms; browser writes also incur
BLE/queue latency. Shape switching needs a 15% margin for two consecutive frames.

**Calibrate Room:** Stay quiet for a two-second countdown and 64 frames of measurement.
Use sorted frame RMS index 57 (nearest-rank 90th percentile), then threshold
`max(1, 1.5 * percentile + 2)`. Commit only on success. Read failures, clipping
(raw samples >=32760 or <=-32760), an entirely zero stream, or a threshold above 1,000
fail without changing settings. Cancellation also preserves settings. Calibration
works with lip sync off, survives temporary expressions and BLE disconnect, and stops
on reboot. It is not speech recognition: talking during measurement can set an
excessive threshold. There is no automatic recalibration or background adaptation.

**Session-only tuning:** Settings and calibration live in RAM. Every reboot restores
threshold 13, mouth gain 1, and Natural response, and starts IDLE. Temporary expressions
and BLE disconnects preserve tuning while the controller remains powered.
There are no viseme NVS reads, writes, save timers, retries, or save indicators.
Previously saved viseme records are ignored and left untouched; unrelated NVS data
is unchanged. Returning to older firmware may read its previously saved record.
Removing persistence reduces status from 19 to 16 bytes; it does not remove a BLE
characteristic because persistence never had a dedicated characteristic.

**Panel:** Noise rejection and mouth size use logarithmic mappings
`low * pow(high/low, t)` for slider fraction t in 0–1. Display pending edits separately
from accepted readback; serialize settings writes and reads and keep only the latest
queued slider update. Response presets adjust timing only; keep Mouth size logarithmic. Use the same linear meter scale for signal and threshold:
`max(100, threshold * 1.25)`. Show actionable calibration/mic failures and explain that reboot resets tuning.

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
| Slot Machine Enable | `a1a2a3a4-b1b2-4c1c-d1d2-e1e2e3e4e5f5` | READ, WRITE | `uint8_t` (0/1) | `1` starts game mode; `0` stops it and restores the normal face. |

While game mode is enabled, a completed proximity boop spins the three reels. Holding the sensor does not repeat a spin; the player must release and boop again. Wins flash the horn LED, losses dim it, and its previous brightness is restored after the result. The proximity sensor cannot start game mode by itself.

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

**Fan Behavior Notes:**
- **Graceful Degradation**: Controller operates normally without fan connected.
- **Safe Defaults**: Fan speed is 0 on boot.
- **Smooth Transitions**: Speed changes use automatic fade transitions.
- **PWM Control**: 4-pin PWM (25kHz, 8-bit) via TXU0202DCUR level shifter.

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
✅ Viseme Settings v2, Calibration, Status v3 (3)
✅ Horn Brightness
✅ Cheek Brightness
✅ Cheek Background Color
✅ Cheek Fade Color
✅ Reboot
✅ Slot Machine Enable
✅ Glitch Trigger
✅ Motion Enable Flags
✅ Tap Sensitivity
✅ Glitch Intensity

### V4 Only
🆕 Fan Speed

**Total Characteristics:**
- V2: 23 characteristics
- V4: 24 characteristics (23 + fan control)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Initial | Original KMMX-Fursuit BLE specification (V2 hardware) |
| 2.0 | 2026-06 | Added V4 hardware support, fan control characteristics, hardware version documentation |
| 2.1 | 2026-07 | Added BLE-controlled slot machine game mode |

Viseme session-only tuning revision: 2026-11-07. See protocol and compatibility above.
