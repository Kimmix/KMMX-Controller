#include "BLE.h"
#include <Arduino.h>
#include "config.h"
#include "BLE_UUIDs.h"

BLEManager* BLEManager::instance = nullptr;

// Constants for BLE advertising
static constexpr uint16_t BLE_ADV_MIN_INTERVAL = 0x20;      // 20ms (0x20 * 0.625ms)
static constexpr uint16_t BLE_ADV_MAX_INTERVAL = 0x40;      // 40ms (0x40 * 0.625ms)
static constexpr uint16_t BLE_APPEARANCE_DISPLAY = 0x03C0;  // Generic Display
static constexpr uint32_t BLE_RW = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE;
static constexpr uint32_t BLE_WRITE = NIMBLE_PROPERTY::WRITE;

// Manufacturer data stored in flash memory (PROGMEM)
static const uint8_t BLE_MFG_DATA[] PROGMEM = {
    0xFF, 0xFF,          // Company ID (0xFFFF = custom/test)
    'K', 'M', 'M', 'X',  // KMMX identifier
    0x01, 0x00           // Version 1.0
};

// Simplified Server Callbacks - directly implement logic
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 100);
        if (BLEManager::instance && BLEManager::instance->debugEnabled) {
            Serial.println(F("[BLE] Client connected"));
        }
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        if (BLEManager::instance && BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Client disconnected, reason: "));
            Serial.println(reason);
        }
    }
};

static bool readByte(NimBLECharacteristic* characteristic, uint8_t& value) {
    const auto data = characteristic->getValue();
    if (data.length() != 1) return false;
    value = data[0];
    return true;
}

// Simplified Characteristic Callbacks - one class per characteristic type
class DisplayBrightnessCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (value > 1) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Display Brightness: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setDisplayBrightness(value);
    }
};

class EyeStateCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Eye State: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setEye(value);
    }
};

class MouthStateCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Mouth State: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setMouth(value);
    }
};

class VisemeCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Viseme: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setViseme(value);
    }
};

class HornBrightnessCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Horn Brightness: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setHornBrightness(value);
    }
};

class CheekBrightnessCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Cheek Brightness: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setCheekBrightness(value);
    }
};

class CheekBgColorCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        if (pCharacteristic->getValue().length() >= 3) {
            const uint8_t* data = pCharacteristic->getValue().data();
            if (BLEManager::instance->debugEnabled) {
                Serial.print(F("[BLE] Cheek BG Color: R="));
                Serial.print(data[0]);
                Serial.print(F(" G="));
                Serial.print(data[1]);
                Serial.print(F(" B="));
                Serial.println(data[2]);
            }
            BLEManager::instance->controller.setCheekBackgroundColor(data[0], data[1], data[2]);
        }
    }
};

class CheekFadeColorCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        if (pCharacteristic->getValue().length() >= 3) {
            const uint8_t* data = pCharacteristic->getValue().data();
            if (BLEManager::instance->debugEnabled) {
                Serial.print(F("[BLE] Cheek Fade Color: R="));
                Serial.print(data[0]);
                Serial.print(F(" G="));
                Serial.print(data[1]);
                Serial.print(F(" B="));
                Serial.println(data[2]);
            }
            BLEManager::instance->controller.setCheekFadeColor(data[0], data[1], data[2]);
        }
    }
};

class DisplayColorModeCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Display Color Mode: "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setDisplayColorMode(value);
    }
};

class DisplayEffectColor1Callbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        if (pCharacteristic->getValue().length() >= 3) {
            const uint8_t* data = pCharacteristic->getValue().data();
            uint8_t r = data[0], g = data[1], b = data[2];
            if (BLEManager::instance->debugEnabled) {
                Serial.print(F("[BLE] Display Effect Color 1: R="));
                Serial.print(r);
                Serial.print(F(" G="));
                Serial.print(g);
                Serial.print(F(" B="));
                Serial.println(b);
            }

            // Get current color 2 to preserve it
            uint8_t color2R, color2G, color2B;
            BLEManager::instance->controller.getDisplayGradientBottomColor(color2R, color2G, color2B);
            BLEManager::instance->controller.setDisplayGradientColors(r, g, b, color2R, color2G, color2B);

            // Also set the dual spiral and dual circle color
            BLEManager::instance->controller.setDisplayDualSpiralColor(r, g, b);
            BLEManager::instance->controller.setDisplayDualCircleColor(r, g, b);
        }
    }
};

class DisplayEffectColor2Callbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        if (pCharacteristic->getValue().length() >= 3) {
            const uint8_t* data = pCharacteristic->getValue().data();
            uint8_t r = data[0], g = data[1], b = data[2];
            if (BLEManager::instance->debugEnabled) {
                Serial.print(F("[BLE] Display Effect Color 2: R="));
                Serial.print(r);
                Serial.print(F(" G="));
                Serial.print(g);
                Serial.print(F(" B="));
                Serial.println(b);
            }

            // Get current color 1 to preserve it
            uint8_t color1R, color1G, color1B;
            BLEManager::instance->controller.getDisplayGradientTopColor(color1R, color1G, color1B);
            BLEManager::instance->controller.setDisplayGradientColors(color1R, color1G, color1B, r, g, b);

            // Also set the dual spiral and dual circle color to match color 1
            BLEManager::instance->controller.setDisplayDualSpiralColor(color1R, color1G, color1B);
            BLEManager::instance->controller.setDisplayDualCircleColor(color1R, color1G, color1B);
        }
    }
};

class DisplayEffectOption1Callbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Display Effect Option 1 (Thickness): "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setDisplayEffectThickness(value);
    }
};

class DisplayEffectOption2Callbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Display Effect Option 2 (Speed): "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setDisplayEffectSpeed(value);
    }
};

class DisplayEffectOption3Callbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Display Effect Option 3 (Direction Inverted): "));
            Serial.println(value);
        }
        BLEManager::instance->controller.setDisplayEffectDirectionInverted(value);
    }
};

class RebootCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t value;
        if (!readByte(pCharacteristic, value)) return;
        if (value != 0) {
            if (BLEManager::instance->debugEnabled) {
                Serial.println(F("[BLE] Reboot requested"));
            }
            BLEManager::instance->controller.reboot();
        }
    }
};

class GlitchTriggerCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t intensity;
        if (!readByte(pCharacteristic, intensity)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Glitch Trigger: "));
            Serial.println(intensity);
        }
        BLEManager::instance->controller.triggerGlitch(intensity);
    }
};

class MotionEnableFlagsCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t flags;
        if (!readByte(pCharacteristic, flags)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Motion Enable Flags: 0x"));
            Serial.println(flags, HEX);
        }
        BLEManager::instance->controller.setMotionEnableFlags(flags);
    }
};

class TapSensitivityCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t sensitivity;
        if (!readByte(pCharacteristic, sensitivity)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Tap Sensitivity: "));
            Serial.println(sensitivity);
        }
        BLEManager::instance->controller.setTapSensitivity(sensitivity);
    }
};

class GlitchIntensityCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t intensity;
        if (!readByte(pCharacteristic, intensity)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Glitch Intensity: "));
            Serial.println(intensity);
        }
        BLEManager::instance->controller.setGlitchIntensity(intensity);
    }
};

// Fan Control Callbacks
#if HAS_FAN_CONTROL
class FanSpeedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t speed;
        if (!readByte(pCharacteristic, speed)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Fan Speed: "));
            Serial.println(speed);
        }
        BLEManager::instance->controller.setFanSpeed(speed);
    }
};

class FanEnabledCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
        if (!BLEManager::instance) return;
        uint8_t enabled;
        if (!readByte(pCharacteristic, enabled)) return;
        if (BLEManager::instance->debugEnabled) {
            Serial.print(F("[BLE] Fan Enabled: "));
            Serial.println(enabled ? "ON" : "OFF");
        }
        BLEManager::instance->controller.setFanEnabled(enabled != 0);
    }
};
#endif

enum class VisemeParameter {
    EnvelopeAttack,
    EnvelopeRelease,
    AttackThreshold,
    NoiseFloorMin,
    AhScale,
    EeScale,
    OhScale,
    OoScale,
    ThScale,
};

class VisemeParameterCallbacks : public NimBLECharacteristicCallbacks {
   public:
    explicit VisemeParameterCallbacks(VisemeParameter parameter) : parameter(parameter) {}

    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) override {
        if (!BLEManager::instance || characteristic->getValue().length() != sizeof(float)) return;

        float value;
        memcpy(&value, characteristic->getValue().data(), sizeof(value));
        if (!isfinite(value)) return;

        auto& controller = BLEManager::instance->controller;
        auto& viseme = controller.viseme();
        switch (parameter) {
            case VisemeParameter::EnvelopeAttack:
                if (value < 0.1f || value > 0.9f) return;
                viseme.setEnvelopeAttack(value);
                break;
            case VisemeParameter::EnvelopeRelease:
                if (value < 0.01f || value > 0.5f) return;
                viseme.setEnvelopeRelease(value);
                break;
            case VisemeParameter::AttackThreshold:
                if (value < 1.0f || value > 3.0f) return;
                viseme.setAttackThreshold(value);
                break;
            case VisemeParameter::NoiseFloorMin:
                if (value < 1.0f || value > visemeNoiseFloorCap) return;
                viseme.setNoiseFloorMin(value);
                break;
            case VisemeParameter::AhScale:
            case VisemeParameter::EeScale:
            case VisemeParameter::OhScale:
            case VisemeParameter::OoScale:
            case VisemeParameter::ThScale:
                if (value < 0.1f || value > 5.0f) return;
                if (parameter == VisemeParameter::AhScale) viseme.setAhScale(value);
                else if (parameter == VisemeParameter::EeScale) viseme.setEeScale(value);
                else if (parameter == VisemeParameter::OhScale) viseme.setOhScale(value);
                else if (parameter == VisemeParameter::OoScale) viseme.setOoScale(value);
                else viseme.setThScale(value);
                break;
        }
    }

   private:
    VisemeParameter parameter;
};

BLEManager& BLEManager::getInstance(KMMXController& ctrl) {
    if (!instance) {
        instance = new BLEManager(ctrl);
    }
    return *instance;
}

BLEManager::BLEManager(KMMXController& ctrl) : controller(ctrl),
                                               pServer(nullptr),
                                               pService(nullptr),
                                               displayBrightnessCharacteristic(nullptr),
                                               eyeStateCharacteristic(nullptr),
                                               mouthStateCharacteristic(nullptr),
                                               visemeCharacteristic(nullptr),
                                               hornBrightnessCharacteristic(nullptr),
                                               cheekBrightnessCharacteristic(nullptr),
                                               cheekBgColorCharacteristic(nullptr),
                                               cheekFadeColorCharacteristic(nullptr),
                                               displayColorModeCharacteristic(nullptr),
                                               displayEffectColor1Characteristic(nullptr),
                                               displayEffectColor2Characteristic(nullptr),
                                               displayEffectOption1Characteristic(nullptr),
                                               displayEffectOption2Characteristic(nullptr),
                                               displayEffectOption3Characteristic(nullptr),
                                               rebootCharacteristic(nullptr),
                                               glitchTriggerCharacteristic(nullptr),
                                               motionEnableFlagsCharacteristic(nullptr),
                                               tapSensitivityCharacteristic(nullptr),
                                               glitchIntensityCharacteristic(nullptr)
#if HAS_FAN_CONTROL
                                               ,fanSpeedCharacteristic(nullptr),
                                               fanEnabledCharacteristic(nullptr),
                                               fanRpmCharacteristic(nullptr),
                                               fanConnectedCharacteristic(nullptr)
#endif
                                               ,visemeEnvelopeAttackCharacteristic(nullptr),
                                               visemeEnvelopeReleaseCharacteristic(nullptr),
                                               visemeAttackThresholdCharacteristic(nullptr),
                                               visemeNoiseFloorMinCharacteristic(nullptr),
                                               visemeAhScaleCharacteristic(nullptr),
                                               visemeEeScaleCharacteristic(nullptr),
                                               visemeOhScaleCharacteristic(nullptr),
                                               visemeOoScaleCharacteristic(nullptr),
                                               visemeThScaleCharacteristic(nullptr)
{
}

void BLEManager::setup() {
    Serial.println(F("Booting BLE..."));

    // Initialize NimBLE with device name
    NimBLEDevice::init(BLE_DEVICE_NAME);

    // Set power level for better range
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // +9dBm

    // Create BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    pServer->advertiseOnDisconnect(true);  // Automatically restart advertising on disconnect

    // Create BLE Service
    pService = pServer->createService(BLE_SERVICE_UUID);

    // Create characteristics with Read & Write properties
    displayBrightnessCharacteristic = pService->createCharacteristic(
        BLE_DISPLAY_BRIGHTNESS_CHARACTERISTIC_UUID,
        BLE_RW);

    eyeStateCharacteristic = pService->createCharacteristic(
        BLE_EYE_STATE_CHARACTERISTIC_UUID,
        BLE_RW);

    mouthStateCharacteristic = pService->createCharacteristic(
        BLE_MOUTH_STATE_CHARACTERISTIC_UUID,
        BLE_RW);

    visemeCharacteristic = pService->createCharacteristic(
        BLE_VISEME_CHARACTERISTIC_UUID,
        BLE_RW);

    hornBrightnessCharacteristic = pService->createCharacteristic(
        BLE_HORN_BRIGHTNESS_CHARACTERISTIC_UUID,
        BLE_RW);

    cheekBrightnessCharacteristic = pService->createCharacteristic(
        BLE_CHEEK_BRIGHTNESS_CHARACTERISTIC_UUID,
        BLE_RW);

    cheekBgColorCharacteristic = pService->createCharacteristic(
        BLE_CHEEK_BG_COLOR_CHARACTERISTIC_UUID,
        BLE_RW);

    cheekFadeColorCharacteristic = pService->createCharacteristic(
        BLE_CHEEK_FADE_COLOR_CHARACTERISTIC_UUID,
        BLE_RW);

    displayColorModeCharacteristic = pService->createCharacteristic(
        BLE_DISPLAY_COLOR_MODE_CHARACTERISTIC_UUID,
        BLE_RW);

    displayEffectColor1Characteristic = pService->createCharacteristic(
        BLE_DISPLAY_EFFECT_COLOR1_CHARACTERISTIC_UUID,
        BLE_RW);

    displayEffectColor2Characteristic = pService->createCharacteristic(
        BLE_DISPLAY_EFFECT_COLOR2_CHARACTERISTIC_UUID,
        BLE_RW);

    displayEffectOption1Characteristic = pService->createCharacteristic(
        BLE_DISPLAY_EFFECT_OPTION1_CHARACTERISTIC_UUID,
        BLE_RW);

    displayEffectOption2Characteristic = pService->createCharacteristic(
        BLE_DISPLAY_EFFECT_OPTION2_CHARACTERISTIC_UUID,
        BLE_RW);

    displayEffectOption3Characteristic = pService->createCharacteristic(
        BLE_DISPLAY_EFFECT_OPTION3_CHARACTERISTIC_UUID,
        BLE_RW);

    rebootCharacteristic = pService->createCharacteristic(
        BLE_REBOOT_CHARACTERISTIC_UUID,
        BLE_WRITE);

    glitchTriggerCharacteristic = pService->createCharacteristic(
        BLE_GLITCH_TRIGGER_CHARACTERISTIC_UUID,
        BLE_WRITE);

    motionEnableFlagsCharacteristic = pService->createCharacteristic(
        BLE_MOTION_ENABLE_FLAGS_CHARACTERISTIC_UUID,
        BLE_RW);

    tapSensitivityCharacteristic = pService->createCharacteristic(
        BLE_TAP_SENSITIVITY_CHARACTERISTIC_UUID,
        BLE_RW);

    glitchIntensityCharacteristic = pService->createCharacteristic(
        BLE_GLITCH_INTENSITY_CHARACTERISTIC_UUID,
        BLE_RW);

    // Fan Control Characteristics
    #if HAS_FAN_CONTROL
    fanSpeedCharacteristic = pService->createCharacteristic(
        BLE_FAN_SPEED_CHARACTERISTIC_UUID,
        BLE_RW);

    fanEnabledCharacteristic = pService->createCharacteristic(
        BLE_FAN_ENABLED_CHARACTERISTIC_UUID,
        BLE_RW);

    fanRpmCharacteristic = pService->createCharacteristic(
        BLE_FAN_RPM_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);  // Read + Notify for real-time updates

    fanConnectedCharacteristic = pService->createCharacteristic(
        BLE_FAN_CONNECTED_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);  // Read + Notify for connection status updates
    #endif

    // Viseme Advanced Parameter Characteristics
    visemeEnvelopeAttackCharacteristic = pService->createCharacteristic(
        BLE_VISEME_ENVELOPE_ATTACK_UUID,
        BLE_RW);

    visemeEnvelopeReleaseCharacteristic = pService->createCharacteristic(
        BLE_VISEME_ENVELOPE_RELEASE_UUID,
        BLE_RW);

    visemeAttackThresholdCharacteristic = pService->createCharacteristic(
        BLE_VISEME_ATTACK_THRESHOLD_UUID,
        BLE_RW);

    visemeNoiseFloorMinCharacteristic = pService->createCharacteristic(
        BLE_VISEME_NOISE_FLOOR_MIN_UUID,
        BLE_RW);

    visemeAhScaleCharacteristic = pService->createCharacteristic(
        BLE_VISEME_AH_SCALE_UUID,
        BLE_RW);

    visemeEeScaleCharacteristic = pService->createCharacteristic(
        BLE_VISEME_EE_SCALE_UUID,
        BLE_RW);

    visemeOhScaleCharacteristic = pService->createCharacteristic(
        BLE_VISEME_OH_SCALE_UUID,
        BLE_RW);

    visemeOoScaleCharacteristic = pService->createCharacteristic(
        BLE_VISEME_OO_SCALE_UUID,
        BLE_RW);

    visemeThScaleCharacteristic = pService->createCharacteristic(
        BLE_VISEME_TH_SCALE_UUID,
        BLE_RW);

    // Set default values for each characteristic
    uint8_t brightnessValue = controller.getDisplayBrightness();
    displayBrightnessCharacteristic->setValue(&brightnessValue, 1);

    uint8_t eyeValue = 0x00;
    eyeStateCharacteristic->setValue(&eyeValue, 1);

    uint8_t mouthValue = 0x00;
    mouthStateCharacteristic->setValue(&mouthValue, 1);

    uint8_t visemeValue = controller.getViseme();
    visemeCharacteristic->setValue(&visemeValue, 1);

    uint8_t hornValue = controller.getHornBrightness();
    hornBrightnessCharacteristic->setValue(&hornValue, 1);

    uint8_t cheekValue = controller.getCheekBrightness();
    cheekBrightnessCharacteristic->setValue(&cheekValue, 1);

    // Set default color values (RGB format)
    uint32_t bgColor = controller.getCheekBackgroundColor();
    uint8_t bgColorData[3] = {(uint8_t)(bgColor >> 16), (uint8_t)(bgColor >> 8), (uint8_t)bgColor};
    cheekBgColorCharacteristic->setValue(bgColorData, 3);

    uint32_t fadeColor = controller.getCheekFadeColor();
    uint8_t fadeColorData[3] = {(uint8_t)(fadeColor >> 16), (uint8_t)(fadeColor >> 8), (uint8_t)fadeColor};
    cheekFadeColorCharacteristic->setValue(fadeColorData, 3);

    // Set display color mode
    uint8_t colorMode = controller.getDisplayColorMode();
    displayColorModeCharacteristic->setValue(&colorMode, 1);

    // Set effect color values
    uint8_t color1R, color1G, color1B, color2R, color2G, color2B;
    controller.getDisplayGradientTopColor(color1R, color1G, color1B);
    controller.getDisplayGradientBottomColor(color2R, color2G, color2B);
    uint8_t color1Data[3] = {color1R, color1G, color1B};
    uint8_t color2Data[3] = {color2R, color2G, color2B};
    displayEffectColor1Characteristic->setValue(color1Data, 3);
    displayEffectColor2Characteristic->setValue(color2Data, 3);

    // Set effect option values
    uint8_t thickness = controller.getDisplayEffectThickness();
    displayEffectOption1Characteristic->setValue(&thickness, 1);

    uint8_t speed = controller.getDisplayEffectSpeed();
    displayEffectOption2Characteristic->setValue(&speed, 1);

    uint8_t direction = controller.getDisplayEffectDirectionInverted();
    displayEffectOption3Characteristic->setValue(&direction, 1);

    // Set motion detection & glitch control default values
    uint8_t motionFlags = controller.getMotionEnableFlags();
    motionEnableFlagsCharacteristic->setValue(&motionFlags, 1);

    uint8_t tapSens = controller.getTapSensitivity();
    tapSensitivityCharacteristic->setValue(&tapSens, 1);

    uint8_t glitchInt = controller.getGlitchIntensity();
    glitchIntensityCharacteristic->setValue(&glitchInt, 1);

    // Set fan control default values
    #if HAS_FAN_CONTROL
    uint8_t fanSpeed = controller.getFanSpeed();
    fanSpeedCharacteristic->setValue(&fanSpeed, 1);

    uint8_t fanEnabled = controller.getFanEnabled() ? 1 : 0;
    fanEnabledCharacteristic->setValue(&fanEnabled, 1);

    uint16_t fanRpm = controller.getFanRPM();
    fanRpmCharacteristic->setValue(reinterpret_cast<uint8_t*>(&fanRpm), 2);  // 2 bytes for RPM

    uint8_t fanConnected = controller.getFanConnected() ? 1 : 0;
    fanConnectedCharacteristic->setValue(&fanConnected, 1);
    #endif

    // Set viseme advanced parameter default values
    auto& viseme = controller.viseme();
    float envAttack = viseme.getEnvelopeAttack();
    visemeEnvelopeAttackCharacteristic->setValue(reinterpret_cast<uint8_t*>(&envAttack), sizeof(float));

    float envRelease = viseme.getEnvelopeRelease();
    visemeEnvelopeReleaseCharacteristic->setValue(reinterpret_cast<uint8_t*>(&envRelease), sizeof(float));

    float attackThresh = viseme.getAttackThreshold();
    visemeAttackThresholdCharacteristic->setValue(reinterpret_cast<uint8_t*>(&attackThresh), sizeof(float));

    float noiseMin = viseme.getNoiseFloorMin();
    visemeNoiseFloorMinCharacteristic->setValue(reinterpret_cast<uint8_t*>(&noiseMin), sizeof(float));

    float ahScale = viseme.getAhScale();
    visemeAhScaleCharacteristic->setValue(reinterpret_cast<uint8_t*>(&ahScale), sizeof(float));

    float eeScale = viseme.getEeScale();
    visemeEeScaleCharacteristic->setValue(reinterpret_cast<uint8_t*>(&eeScale), sizeof(float));

    float ohScale = viseme.getOhScale();
    visemeOhScaleCharacteristic->setValue(reinterpret_cast<uint8_t*>(&ohScale), sizeof(float));

    float ooScale = viseme.getOoScale();
    visemeOoScaleCharacteristic->setValue(reinterpret_cast<uint8_t*>(&ooScale), sizeof(float));

    float thScale = viseme.getThScale();
    visemeThScaleCharacteristic->setValue(reinterpret_cast<uint8_t*>(&thScale), sizeof(float));

    // Set callbacks for each characteristic (simple, direct callbacks)
    displayBrightnessCharacteristic->setCallbacks(new DisplayBrightnessCallbacks());
    eyeStateCharacteristic->setCallbacks(new EyeStateCallbacks());
    mouthStateCharacteristic->setCallbacks(new MouthStateCallbacks());
    visemeCharacteristic->setCallbacks(new VisemeCallbacks());
    hornBrightnessCharacteristic->setCallbacks(new HornBrightnessCallbacks());
    cheekBrightnessCharacteristic->setCallbacks(new CheekBrightnessCallbacks());
    cheekBgColorCharacteristic->setCallbacks(new CheekBgColorCallbacks());
    cheekFadeColorCharacteristic->setCallbacks(new CheekFadeColorCallbacks());
    displayColorModeCharacteristic->setCallbacks(new DisplayColorModeCallbacks());
    displayEffectColor1Characteristic->setCallbacks(new DisplayEffectColor1Callbacks());
    displayEffectColor2Characteristic->setCallbacks(new DisplayEffectColor2Callbacks());
    displayEffectOption1Characteristic->setCallbacks(new DisplayEffectOption1Callbacks());
    displayEffectOption2Characteristic->setCallbacks(new DisplayEffectOption2Callbacks());
    displayEffectOption3Characteristic->setCallbacks(new DisplayEffectOption3Callbacks());
    rebootCharacteristic->setCallbacks(new RebootCallbacks());
    glitchTriggerCharacteristic->setCallbacks(new GlitchTriggerCallbacks());
    motionEnableFlagsCharacteristic->setCallbacks(new MotionEnableFlagsCallbacks());
    tapSensitivityCharacteristic->setCallbacks(new TapSensitivityCallbacks());
    glitchIntensityCharacteristic->setCallbacks(new GlitchIntensityCallbacks());

    // Set fan control callbacks
    #if HAS_FAN_CONTROL
    fanSpeedCharacteristic->setCallbacks(new FanSpeedCallbacks());
    fanEnabledCharacteristic->setCallbacks(new FanEnabledCallbacks());
    // RPM characteristic is read-only (no callbacks needed)
    #endif

    // Set viseme advanced parameter callbacks
    visemeEnvelopeAttackCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::EnvelopeAttack));
    visemeEnvelopeReleaseCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::EnvelopeRelease));
    visemeAttackThresholdCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::AttackThreshold));
    visemeNoiseFloorMinCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::NoiseFloorMin));
    visemeAhScaleCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::AhScale));
    visemeEeScaleCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::EeScale));
    visemeOhScaleCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::OhScale));
    visemeOoScaleCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::OoScale));
    visemeThScaleCharacteristic->setCallbacks(new VisemeParameterCallbacks(VisemeParameter::ThScale));

    // Start the server (this automatically starts all services)
    pServer->start();

    // Configure advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    // Create advertising data with manufacturer info and appearance
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);  // General discoverable, BR/EDR not supported
    advData.setCompleteServices(BLEUUID(BLE_SERVICE_UUID));
    advData.setAppearance(BLE_APPEARANCE_DISPLAY);

    // Add manufacturer data from flash memory
    uint8_t mfgDataBuffer[sizeof(BLE_MFG_DATA)];
    memcpy_P(mfgDataBuffer, BLE_MFG_DATA, sizeof(BLE_MFG_DATA));
    advData.setManufacturerData(std::string((char*)mfgDataBuffer, sizeof(mfgDataBuffer)));

    // Create scan response data with device name
    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName(BLE_DEVICE_NAME);

    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponseData);
    pAdvertising->setMinInterval(BLE_ADV_MIN_INTERVAL);
    pAdvertising->setMaxInterval(BLE_ADV_MAX_INTERVAL);

    // Start advertising
    if (pAdvertising->start()) {
        Serial.println(F("Bluetooth® device active, waiting for connections..."));
    } else {
        Serial.println(F("Failed to start advertising!"));
    }
}

bool BLEManager::isConnected() const {
    return pServer && pServer->getConnectedCount() > 0;
}

void BLEManager::update() {
#if HAS_FAN_CONTROL
    static uint32_t lastUpdate = 0;
    static uint16_t lastRpm = UINT16_MAX;
    static uint8_t lastConnected = UINT8_MAX;
    if (!isConnected() || millis() - lastUpdate < 500) return;
    lastUpdate = millis();

    const uint16_t rpm = controller.getFanRPM();
    const uint8_t connected = controller.getFanConnected();
    if (rpm != lastRpm) {
        lastRpm = rpm;
        fanRpmCharacteristic->setValue(reinterpret_cast<const uint8_t*>(&rpm), sizeof(rpm));
        fanRpmCharacteristic->notify();
    }
    if (connected != lastConnected) {
        lastConnected = connected;
        fanConnectedCharacteristic->setValue(&connected, sizeof(connected));
        fanConnectedCharacteristic->notify();
    }
#endif
}
