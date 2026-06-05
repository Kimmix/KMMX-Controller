#pragma once
#include <Arduino.h>
#include <memory>

#include "config.h"
#include "Utils/Utils.h"
#include "Utils/FPSCounter.h"

#include "FacialStates/EyeState/EyeState.h"
#include "FacialStates/MouthState/MouthState.h"
#include "FacialStates/FXState/FXState.h"

#include "Renderer/Boop.h"

#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Devices/HornLED/HornLED.h"
#include "Devices/Accelerometer/LIS3DH.h"
#include "Devices/Proximity/IProximitySensor.h"
#include "Devices/Ws2812/RGBStatus.h"
#include "Devices/Ws2812/CheekPanel.h"
#include "Devices/OLEDDisplay/SSD1306.h"
#include "Types/SensorData.h"

class KMMXController {
   public:
    void setupSensors();
    void update();
    void setEye(int i);
    void setMouth(int i);
    void setViseme(int b);
    int getViseme();
    int getDisplayBrightness();
    void setDisplayBrightness(int i);
    int getHornBrightness();
    void setHornBrightness(int i);
    int getCheekBrightness();
    void setCheekBrightness(int i);
    void setCheekBackgroundColor(uint8_t r, uint8_t g, uint8_t b);
    void setCheekFadeColor(uint8_t r, uint8_t g, uint8_t b);
    uint32_t getCheekBackgroundColor();
    uint32_t getCheekFadeColor();
    void setDisplayColorMode(uint8_t mode);
    uint8_t getDisplayColorMode();
    void setDisplayGradientColors(uint8_t topR, uint8_t topG, uint8_t topB, uint8_t bottomR, uint8_t bottomG, uint8_t bottomB);
    void getDisplayGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void getDisplayGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void setDisplayDualSpiralColor(uint8_t spiralR, uint8_t spiralG, uint8_t spiralB);
    void getDisplayDualSpiralColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void setDisplayDualCircleColor(uint8_t circleR, uint8_t circleG, uint8_t circleB);
    void getDisplayDualCircleColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void setDisplayEffectThickness(uint8_t thickness);
    uint8_t getDisplayEffectThickness();
    void setDisplayEffectSpeed(uint8_t speed);
    uint8_t getDisplayEffectSpeed();
    void setDisplayEffectDirectionInverted(uint8_t inverted);
    uint8_t getDisplayEffectDirectionInverted();
    void reboot();

    // Motion Detection & Glitch Control (BLE)
    void triggerGlitch(uint8_t intensity);                      // Manually trigger glitch effect with intensity 0-100
    void setMotionEnableFlags(uint8_t flags);                   // Set motion detection enable flags (bitfield)
    uint8_t getMotionEnableFlags();                             // Get current motion detection enable flags
    void setTapSensitivity(uint8_t sensitivity);                // Set tap sensitivity 0-100 (maps to threshold)
    uint8_t getTapSensitivity();                                // Get current tap sensitivity as 0-100
    void setGlitchIntensity(uint8_t intensity);                 // Set glitch intensity 0-100 (affects min/max range)
    uint8_t getGlitchIntensity();                               // Get current glitch intensity as 0-100

    // FPS counter access
    float getFPS() const;
    int getTargetFPS() const;

   private:
    // Devices
    Hub75DMA display;
    RGBStatus statusLED = RGBStatus(RGB_STATUS_PIN);
    CheekPanel cheekPanel = CheekPanel(argbCount, ARGB_PIN);
    HornLED hornLED;
    LIS3DH accelerometer;
    std::unique_ptr<IProximitySensor> proximitySensor;  // Auto-detected proximity sensor (VL6180X or APDS9930)
    SSD1306 oledDisplay;
    // Double-buffer for thread-safe sensor access
    SensorData sensorBuffer[2];
    volatile uint8_t activeBuffer = 0;
    TaskHandle_t sensorTaskHandle;
    TaskHandle_t renderTaskHandle;
    // Renderer states
    EyeState eyeState = EyeState(&display);
    MouthState mouthState = MouthState(&display);
    FXState fxState = FXState(&display);
    Boop boop;
    int16_t pixelPos = 0;

    // Performance tracking
    FPSCounter fpsCounter;

    // Sensor initialization helpers
    bool initializeAccelerometer();
    bool initializeProximitySensor();
    bool initializeOLED();
    void printSensorStatus();

    void renderFace();
    void handleBoop();
    void handleBoopCompleted(float speed, bool eyeAlreadyActive);
    void handleBoopInRange();
    void handleBoopContinuous(bool eyeAlreadyActive);
    void handleBoopAngry();
    void handleBoopIdle(bool wasInRange, bool wasBooped);
    template<typename StateType, typename EnumType>
    void setStateIfDifferent(StateType& state, EnumType targetState, unsigned long timeout);
    void updateOLED();
    void drawOLEDFaceMirror(const SensorData& sensors);
    void drawOLEDSensorBars(const SensorData& sensors);
    void drawOLEDBluetooth();
    void drawOLEDAccelerometer(const SensorData& sensors);
    void drawOLEDStateNames();
    void drawOLEDFPS();
    void resetIdleTime(KMMXController *controller);
    void resetIdleTime();
    void enterSleep(KMMXController *controller);
    void checkIdleAndSleep(KMMXController *controller, unsigned long currentTime);
    static void readSensorTask(void *parameter);
    static void renderTask(void *parameter);
    const SensorData& getSensorData() const;

    // Motion detection methods
    void checkMotionFeatures(KMMXController *controller);
    void detectTilt(const SensorData& current);
    void detectUpsideDown(const SensorData& current);
    void detectPetting(const SensorData& current);
    void detectTap(const SensorData& current);
    void triggerTiltResponse(float angle, bool isLeftRight);
    void triggerUpsideDownResponse();
    void triggerPettingResponse();  // Removed unused 'sustained' parameter
    void triggerTapResponse(float tapMagnitude);

    // Motion detection helper methods
    inline bool hasDebounceExpired(unsigned long lastTime, uint16_t debounceTime) const;
    void restorePreviousState(EyeStateEnum prevEye, MouthStateEnum prevMouth);
    void resetTiltToNeutral(unsigned long currentTime, bool wasForwardBack);
    bool canSwitchTiltDirection(unsigned long currentTime);
    void handleActiveTiltState(unsigned long currentTime, bool isNeutral,
                               bool isTiltedForwardBack, bool isTiltedLeftRight,
                               float tiltX, float tiltZ);
    void handleTiltTracking(unsigned long currentTime, bool isNeutral,
                           bool isTiltedForwardBack, bool isTiltedLeftRight,
                           float tiltX, float tiltZ);

    // Previous sensor values for idle detection
    SensorData prevSensorData;
    SensorData baselineAccel;  // Baseline acceleration when motion was last detected
    unsigned long stillTime = 0;  // Time when the accelerometer became still
    uint8_t motionCounter = 0;  // Counter for hysteresis (sustained motion detection)
    bool baselineInitialized = false;  // Whether baseline has been set
    unsigned long nextFrame;
    unsigned long nextBoop = 0;
    bool isSleeping = false;
    bool accelerometerInitialized = false;  // Track if accelerometer successfully initialized
    bool oledInitialized = false;  // Track if OLED successfully initialized
    bool boopInitialized = false;
    unsigned short prevHornBright = hornBrightness;
    // Motion detection state structures
    struct TiltDetector {
        float tiltAngleX = 0.0f;  // Forward/back tilt
        float tiltAngleZ = 0.0f;  // Left/right tilt
        unsigned long tiltStartTime = 0;
        unsigned long lastTiltChangeTime = 0;
        unsigned long lastForwardBackTime = 0;  // Track last forward/back tilt time
        unsigned long lastNeutralReturnTime = 0;  // Track when tilt returns to neutral (for petting cooldown)
        bool isTilted = false;
        bool isLeftRight = false;  // true = left/right, false = forward/back
        EyeStateEnum previousEyeState = EyeStateEnum::IDLE;
        MouthStateEnum previousMouthState = MouthStateEnum::IDLE;
    } tiltDetector;

    struct UpsideDownDetector {
        unsigned long upsideDownStartTime = 0;
        unsigned long lastStateChangeTime = 0;
        bool isUpsideDown = false;
        EyeStateEnum previousEyeState = EyeStateEnum::IDLE;
        MouthStateEnum previousMouthState = MouthStateEnum::IDLE;
    } upsideDownDetector;

    struct PettingDetector {
        // Spike detection fields
        unsigned long lastSpikeTime = 0;    // Time of last detected spike (for cooldown)
        float lastMagnitude = 0.0f;         // Previous magnitude reading (for spike detection)

        // Dynamic happiness system
        float happiness = 0.0f;             // Current happiness level (0-100)
        unsigned long lastUpdateTime = 0;   // Last time happiness was updated (for decay calculation)

        // Response state
        bool isPetting = false;             // Currently showing petting response
        EyeStateEnum previousEyeState = EyeStateEnum::IDLE;
        MouthStateEnum previousMouthState = MouthStateEnum::IDLE;
    } pettingDetector;

    struct TapDetector {
        unsigned long lastTapTime = 0;      // Time of last detected tap (for cooldown)
        float lastMagnitude = 0.0f;         // Previous magnitude reading (for spike detection)
        float lastTapMagnitude = 0.0f;      // Last detected tap magnitude (for HUD display)
        unsigned long lastTapDisplayTime = 0; // Time of last tap (for fade-out effect on HUD)

        // Peak + decay verification fields
        float peakMagnitude = 0.0f;         // Peak magnitude detected during tap
        unsigned long peakTime = 0;         // Time when peak was detected
        bool waitingForDecay = false;       // True when waiting to verify decay after peak
    } tapDetector;
};