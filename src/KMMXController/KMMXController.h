#pragma once
#include <Arduino.h>
#include <memory>
#include <atomic>
#include <freertos/queue.h>

#include "config.h"
#include "Utils/Utils.h"
#include "Utils/FPSCounter.h"

#include "FacialStates/EyeState/EyeState.h"
#include "FacialStates/MouthState/MouthState.h"
#include "FacialStates/FXState/FXState.h"

#include "Renderer/Boop.h"
#include "Games/SlotMachine.h"

#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Devices/HornLED/HornLED.h"

// Conditional accelerometer based on board capabilities
#if ACCEL_TYPE_MPU6050
    #include "Devices/Accelerometer/MPU6050.h"
    typedef MPU6050 AccelerometerType;
#elif ACCEL_TYPE_LIS3DH
    #include "Devices/Accelerometer/LIS3DH.h"
    typedef LIS3DH AccelerometerType;
#else
    #error "No accelerometer type defined! Check board pin configuration."
#endif

#if HAS_FAN_CONTROL
    #include "Devices/Fan/Fan.h"
#endif

#include "Devices/Proximity/IProximitySensor.h"
#include "Devices/Ws2812/RGBStatus.h"
#include "Devices/Ws2812/CheekPanel.h"
#include "Devices/OLEDDisplay/SSD1306.h"
#include "Types/SensorData.h"
#include "Types/Orientation.h"

class KMMXController {
   public:
    void setupSensors();
    void update();
    void setEye(int i);
    void setMouth(int i);
    void setViseme(int b);
    int getViseme();
    Viseme& viseme() { return mouthState.viseme; }
    Hub75DMA& displayControl() { return display; }
    Orientation::Snapshot getOrientation();
    int getHornBrightness();
    void setHornBrightness(int i);
    int getCheekBrightness();
    void setCheekBrightness(int i);
    void setCheekBackgroundColor(uint8_t r, uint8_t g, uint8_t b);
    void setCheekFadeColor(uint8_t r, uint8_t g, uint8_t b);
    uint32_t getCheekBackgroundColor();
    uint32_t getCheekFadeColor();
    void reboot();
    void setSlotMachineEnabled(int enabled);
    int getSlotMachineEnabled() const;

    // Motion Detection & Glitch Control (BLE)
    void triggerGlitch(int intensity);                         // Manually trigger glitch effect with intensity 0-100
    void setMotionEnableFlags(uint8_t flags);                   // Set motion detection enable flags (bitfield)
    uint8_t getMotionEnableFlags();                             // Get current motion detection enable flags
    void setTapSensitivity(uint8_t sensitivity);                // Set tap sensitivity 0-100 (maps to threshold)
    uint8_t getTapSensitivity();                                // Get current tap sensitivity as 0-100
    void setGlitchIntensity(uint8_t intensity);                 // Set glitch intensity 0-100 (affects min/max range)
    uint8_t getGlitchIntensity();                               // Get current glitch intensity as 0-100

    // FPS counter access
    float getFPS() const;
    int getTargetFPS() const;

    // Fan Control
    #if HAS_FAN_CONTROL
    void setFanSpeed(int speed);                                    // Set fan speed 0-100
    int getFanSpeed();                                              // Get current fan speed
    #endif

   private:
    // Devices
    Hub75DMA display;
    RGBStatus statusLED = RGBStatus(RGB_STATUS_PIN);
    CheekPanel cheekPanel = CheekPanel(argbCount, ARGB_PIN);
    HornLED hornLED;
    AccelerometerType accelerometer;  // LIS3DH or MPU6050 based on board capabilities
    std::unique_ptr<IProximitySensor> proximitySensor;  // Auto-detected proximity sensor (VL6180X or APDS9930)
    SSD1306 oledDisplay;
    #if HAS_FAN_CONTROL
    Fan fan;  // Fan controller
    #endif
    SensorData currentSensor;  // latest sample, written/read on the render task only
    portMUX_TYPE orientationMux = portMUX_INITIALIZER_UNLOCKED;
    Orientation::Snapshot orientation = {ACCEL_TYPE_MPU6050 ? Orientation::Unavailable : Orientation::Unsupported};
    TaskHandle_t sensorTaskHandle;
    TaskHandle_t renderTaskHandle = nullptr;
    using ControlHandler = void (KMMXController::*)(int);
    struct RenderCommand { ControlHandler handler; int value; SensorData sample; };
    static constexpr unsigned commandCapacity = 16;
    StaticQueue_t commandQueueStorage;
    uint8_t commandStorage[commandCapacity * sizeof(RenderCommand)];
    QueueHandle_t commandQueue = nullptr;
    std::atomic<bool> renderRunning{false};
    mutable portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;
    SensorData sensorSnapshot;
    bool queueControl(ControlHandler handler, int value);
    void acceptSensor(const SensorData& sample);
    // Renderer states
    EyeState eyeState = EyeState(&display);
    MouthState mouthState = MouthState(&display);
    FXState fxState = FXState(&display);
    Boop boop;
    SlotMachine slotMachine = SlotMachine(&display);

    // Performance tracking
    FPSCounter fpsCounter;

    // Sensor initialization helpers
    bool initializeAccelerometer();
    bool initializeProximitySensor();
    bool initializeOLED();
    void printSensorStatus();

    void handleBoop();
    void updateSlotMachineHorn();
    void updateSlotMachineCheeks();
    template<typename StateType, typename EnumType>
    void setStateIfDifferent(StateType& state, EnumType targetState, unsigned long timeout);
    void updateOLED();
    void drawOLEDSlotMachine();
    void drawOLEDFaceMirror();
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
    SensorData getSensorData() const;

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
    SensorData baselineAccel;  // Baseline acceleration when motion was last detected
    unsigned long stillTime = 0;  // Time when the accelerometer became still
    uint8_t motionCounter = 0;  // Counter for hysteresis (sustained motion detection)
    bool baselineInitialized = false;  // Whether baseline has been set
    bool boopLockedOut = false;  // runtime lock-out after sustained too-close; not the user setting
    unsigned long lastBoopUpdate = static_cast<unsigned long>(-50);
    bool isSleeping = false;
    bool accelerometerInitialized = false;  // Track if accelerometer successfully initialized
    bool oledInitialized = false;  // Track if OLED successfully initialized
    bool boopInitialized = false;
    bool slotMachineWasEnabled = false;
    SlotMachine::Outcome slotHornOutcome = SlotMachine::Outcome::NONE;
    int slotHornBrightness = hornBrightness;
    unsigned long lastSlotHornFlash = 0;
    unsigned long slotHornCueUntil = 0;
    uint8_t slotHornStoppedReels = 0;
    bool slotHornAnticipating = false;
    bool slotHornWasEnabled = false;
    bool slotHornFlashOn = false;
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
        uint8_t previousCheekBrightness = 0;
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
