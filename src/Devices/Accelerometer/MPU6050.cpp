#include "MPU6050.h"
#include <Arduino.h>

MPU6050::MPU6050() : mpu(Adafruit_MPU6050()) {}

bool MPU6050::setUp() {
    sensorInitialized = mpu.begin(0x68);

    if (sensorInitialized) {
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_250_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("MPU6050 configured: ±2G accel, ±250°/s gyro, ~50Hz rate");

        // Calibrate gyroscope offsets
        Serial.print("Calibrating gyroscope");
        calibrateGyro();
        Serial.println(" - Done!");

        // Initialize timing for sensor fusion
        lastUpdateTime = millis();
    }

    return sensorInitialized;
}

sensors_event_t* MPU6050::getSensorEvent() {
    if (sensorInitialized) {
        // Note: We don't call mpu.getEvent() here anymore
        // update() should be called instead, which reads both sensors and updates fusion
        // This just returns the cached accel data
    } else {
        // Return safe default values if sensor not initialized
        // Simulate normal gravity when upright (same as LIS3DH default)
        accelEvent.acceleration.x = 0.0f;
        accelEvent.acceleration.y = -9.8f;  // Default gravity when upright
        accelEvent.acceleration.z = 0.0f;
    }

    return &accelEvent;
}

sensors_event_t* MPU6050::getGyroEvent() {
    // Return cached calibrated gyroscope data
    return &gyroEvent;
}

void MPU6050::getFusedOrientation(float& pitch, float& roll, float& yaw) {
    pitch = fusedPitch;
    roll = fusedRoll;
    yaw = fusedYaw;
}

void MPU6050::update() {
    if (!sensorInitialized) {
        return;
    }

    // Read both accelerometer and gyroscope in one I2C transaction (optimized!)
    mpu.getEvent(&accelEvent, &gyroEvent, &tempEvent);

    // Apply gyroscope calibration offsets
    gyroEvent.gyro.x -= gyroOffsetX;
    gyroEvent.gyro.y -= gyroOffsetY;
    gyroEvent.gyro.z -= gyroOffsetZ;

    // Calculate delta time for sensor fusion
    unsigned long currentTime = millis();
    float dt = (currentTime - lastUpdateTime) / 1000.0f;  // Convert to seconds
    lastUpdateTime = currentTime;

    // Clamp dt to prevent issues during startup or time jumps
    if (dt > 0.1f || dt <= 0.0f) {
        dt = 0.02f;  // Default to 50Hz (20ms)
    }

    // Update complementary filter for orientation
    updateSensorFusion(dt);
}

// ============================================================================
// CALIBRATION
// ============================================================================

void MPU6050::calibrateGyro() {
    if (!sensorInitialized) {
        return;
    }

    const int numSamples = 200;  // Collect 200 samples (~4 seconds at 50Hz)
    float sumX = 0, sumY = 0, sumZ = 0;

    sensors_event_t accel, gyro, temp;

    for (int i = 0; i < numSamples; i++) {
        mpu.getEvent(&accel, &gyro, &temp);
        sumX += gyro.gyro.x;
        sumY += gyro.gyro.y;
        sumZ += gyro.gyro.z;

        // Print progress dots
        if (i % 40 == 0) {
            Serial.print(".");
        }

        delay(20);  // 50Hz sampling
    }

    // Calculate average offsets
    gyroOffsetX = sumX / numSamples;
    gyroOffsetY = sumY / numSamples;
    gyroOffsetZ = sumZ / numSamples;

    Serial.printf("\nGyro offsets: X=%.4f, Y=%.4f, Z=%.4f rad/s\n",
                  gyroOffsetX, gyroOffsetY, gyroOffsetZ);
}

// ============================================================================
// SENSOR FUSION - Complementary Filter
// ============================================================================

void MPU6050::updateSensorFusion(float dt) {
    // Get accelerometer angles (in degrees)
    // These are noisy but don't drift over time
    float accelPitch = atan2(accelEvent.acceleration.y,
                             sqrt(accelEvent.acceleration.x * accelEvent.acceleration.x +
                                  accelEvent.acceleration.z * accelEvent.acceleration.z)) * 57.2958f;
    float accelRoll = atan2(-accelEvent.acceleration.x, accelEvent.acceleration.z) * 57.2958f;

    // Integrate gyroscope data (in degrees)
    // These are smooth but drift over time
    // Convert gyro from rad/s to deg/s and integrate
    float gyroPitchDelta = gyroEvent.gyro.x * 57.2958f * dt;
    float gyroRollDelta = gyroEvent.gyro.y * 57.2958f * dt;
    float gyroYawDelta = gyroEvent.gyro.z * 57.2958f * dt;

    // Complementary filter: blend accelerometer and gyroscope
    // fusedAngle = ALPHA * accelAngle + (1 - ALPHA) * (fusedAngle + gyroRate * dt)
    fusedPitch = ALPHA * accelPitch + (1.0f - ALPHA) * (fusedPitch + gyroPitchDelta);
    fusedRoll = ALPHA * accelRoll + (1.0f - ALPHA) * (fusedRoll + gyroRollDelta);

    // Yaw cannot be corrected by accelerometer (no magnetometer), so it will drift
    // We still track it for rotation rate detection
    fusedYaw += gyroYawDelta;

    // Keep yaw in -180 to +180 range
    if (fusedYaw > 180.0f) fusedYaw -= 360.0f;
    if (fusedYaw < -180.0f) fusedYaw += 360.0f;
}

// ============================================================================
// TEMPERATURE SENSOR
// ============================================================================

float MPU6050::getTemperature() {
    if (!sensorInitialized) {
        return 0.0f;
    }
    return tempEvent.temperature;
}
