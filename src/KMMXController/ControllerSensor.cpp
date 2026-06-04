#include "KMMXController.h"
#include <cmath>
#include "esp_dsp.h"
#include "Devices/Proximity/VL6180XSensor.h"
#include "Devices/Proximity/APDS9930Sensor.h"

void KMMXController::setupSensors() {
    // Initialize I2C bus for all sensors
    Wire.begin(S3_SDA, S3_SCL);
    Wire.setClock(400000);  // Set I2C to 400kHz Fast Mode (default is 100kHz) for faster sensor reads

    // Initialize non-I2C devices (always succeed)
    statusLED.init();
    cheekPanel.configure(sideColor1RGB, sideColor2RGB, sideLEDFadeInterval);
    cheekPanel.init();

    // Initialize I2C sensors (plug-and-play - gracefully handle missing devices)
    accelerometerInitialized = initializeAccelerometer();
    boopInitialized = initializeProximitySensor();
    oledInitialized = initializeOLED();

    // Print summary of all sensor statuses
    printSensorStatus();

    // Ensure eye state starts at IDLE (prevents lingering states from previous sessions)
    eyeState.setState(EyeStateEnum::IDLE, true, 0);      // Persistent, no timeout (initial state)
    mouthState.setState(MouthStateEnum::IDLE, true, 0);  // Persistent, no timeout (initial state)

    // Connect FXState to EyeState for automatic blushingFX rendering
    fxState.setEyeStatePtr(&eyeState);

    // Create sensor reading task on Core 1 (separate from rendering for better performance)
    // This prevents slow I2C sensors (like VL6180X) from blocking the render task
    xTaskCreatePinnedToCore(readSensorTask, "SensorTask", 4096, this, 2, &sensorTaskHandle, 1);

    // Create rendering task on Core 0 (dedicated core for smooth rendering)
    // Priority 1 - lower priority since it has the whole core to itself
    xTaskCreatePinnedToCore(renderTask, "RenderTask", 4096, this, 1, &renderTaskHandle, 0);

    Serial.println("=== Task Initialization Complete ===");
}

void KMMXController::readSensorTask(void* parameter) {
    KMMXController* ctrl = static_cast<KMMXController*>(parameter);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    constexpr TickType_t xFrequency = pdMS_TO_TICKS(sensorUpdateInterval);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        uint8_t writeBuffer = 1 - ctrl->activeBuffer;
        SensorData& data = ctrl->sensorBuffer[writeBuffer];

        // Read accelerometer (handles uninitialized state internally)
        sensors_event_t* event = ctrl->accelerometer.getSensorEvent();
        data.accelX = event->acceleration.x;
        data.accelY = event->acceleration.y;
        data.accelZ = event->acceleration.z;

        // Calculate magnitude using ESP-DSP (hardware-accelerated)
        float accelVec[3] = {data.accelX, data.accelY, data.accelZ};
        float dotProduct;
        dsps_dotprod_f32(accelVec, accelVec, &dotProduct, 3);
        data.accelMagnitude = sqrtf(dotProduct);

        // Read proximity (handles nullptr internally)
        data.proximity = ctrl->proximitySensor ? (ctrl->proximitySensor->read(&data.proximity), data.proximity) : 0;

        // Atomic buffer swap
        ctrl->prevSensorData = ctrl->sensorBuffer[ctrl->activeBuffer];
        ctrl->activeBuffer = writeBuffer;

        // Motion detection (only if accelerometer initialized)
        if (ctrl->accelerometerInitialized) {
            ctrl->checkIdleAndSleep(ctrl, millis());
            ctrl->checkMotionFeatures(ctrl);
        }

        // Update facial states
        const SensorData& current = ctrl->sensorBuffer[ctrl->activeBuffer];
        ctrl->mouthState.setSensorData(current);
        ctrl->eyeState.setSensorData(current);
    }
}

const SensorData& KMMXController::getSensorData() const {
    return sensorBuffer[activeBuffer];
}

// ===========================
// Sensor Initialization
// ===========================

bool KMMXController::initializeAccelerometer() {
    Serial.println("=== Accelerometer (LIS3DH) ===");
    bool success = accelerometer.setUp();
    Serial.println(success ? "✓ OK" : "✗ DISABLED - motion detection off");
    return success;
}

bool KMMXController::initializeProximitySensor() {
    Serial.println("=== Proximity Sensor ===");

    // Try VL6180X first (more accurate)
    std::unique_ptr<VL6180XSensor> vl6180x(new VL6180XSensor());
    if (vl6180x->setup()) {
        proximitySensor = std::move(vl6180x);
        Serial.println("✓ VL6180X OK");
        return true;
    }

    // Fallback to APDS9930
    std::unique_ptr<APDS9930Sensor> apds(new APDS9930Sensor());
    if (apds->setup()) {
        proximitySensor = std::move(apds);
        Serial.println("✓ APDS9930 OK");
        return true;
    }

    Serial.println("✗ DISABLED - boop detection off");
    return false;
}

bool KMMXController::initializeOLED() {
    Serial.println("=== OLED Display (SSD1306) ===");
    bool success = oledDisplay.setup();
    Serial.println(success ? "✓ OK" : "✗ DISABLED - HUD off");
    return success;
}

void KMMXController::printSensorStatus() {
    const char* sensors[] = {"Accelerometer", "Proximity", "OLED"};
    const bool states[] = {accelerometerInitialized, boopInitialized, oledInitialized};
    int disabled = 0;

    Serial.println("\n=== Sensor Summary ===");
    for (int i = 0; i < 3; i++) {
        Serial.printf("%s: %s\n", sensors[i], states[i] ? "✓" : "✗");
        if (!states[i]) disabled++;
    }

    Serial.println(disabled == 0 ? "\n✓ All sensors operational" : disabled == 3 ? "\n⚠ All sensors disabled"
                                                                                 : "\n⚠ Reduced functionality");
    Serial.println("======================\n");
}
