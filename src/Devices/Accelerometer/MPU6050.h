#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "IAccelerometer.h"

/**
 * @brief MPU6050 6-Axis IMU implementation for KimmixControllerV4
 *
 * This class wraps the Adafruit MPU6050 library to provide both accelerometer
 * and gyroscope functionality with sensor fusion for accurate orientation tracking.
 */
class MPU6050 : public IAccelerometer {
private:
    sensors_event_t accelEvent;     // Cached acceleration event
    sensors_event_t gyroEvent;      // Cached gyroscope event
    sensors_event_t tempEvent;      // Temperature event (available for future use)
    Adafruit_MPU6050 mpu;           // MPU6050 driver instance
    bool sensorInitialized = false; // Initialization status

    // Gyroscope calibration offsets (rad/s)
    float gyroOffsetX = 0.0f;
    float gyroOffsetY = 0.0f;
    float gyroOffsetZ = 0.0f;

    // Sensor fusion state (complementary filter)
    float fusedPitch = 0.0f;  // Fused pitch angle (degrees)
    float fusedRoll = 0.0f;   // Fused roll angle (degrees)
    float fusedYaw = 0.0f;    // Fused yaw angle (degrees) - will drift without magnetometer
    unsigned long lastUpdateTime = 0;  // For delta time calculation

    // Complementary filter coefficient (0.0 = trust gyro only, 1.0 = trust accel only)
    // Typical value: 0.02-0.05 for 50Hz update rate
    static constexpr float ALPHA = 0.04f;

    // Helper methods for sensor fusion
    void calibrateGyro();  // Auto-calibrate gyroscope offsets
    void updateSensorFusion(float dt);  // Update complementary filter
    float getTemperature();  // Get temperature reading (°C)

public:
    /**
     * @brief Construct a new MPU6050 IMU object
     */
    MPU6050();

    /**
     * @brief Initialize the MPU6050 sensor with calibration
     *
     * Configures the sensor with:
     * - I2C address 0x68
     * - Data rate ~50Hz
     * - Accelerometer range ±2G
     * - Gyroscope range ±250°/s
     * - Auto-calibrates gyroscope offsets
     *
     * @return true if initialization successful, false otherwise
     */
    bool setUp() override;

    /**
     * @brief Get the latest acceleration sensor reading
     *
     * Returns acceleration data on all three axes (x, y, z) in m/s².
     * If the sensor is not initialized, returns safe default values
     * (0, -9.8, 0) representing normal gravity when upright.
     *
     * @return Pointer to sensors_event_t structure with acceleration data
     */
    sensors_event_t* getSensorEvent() override;

    /**
     * @brief Get the latest gyroscope sensor reading
     *
     * Returns calibrated gyroscope data (rotation rates) on all three axes in rad/s.
     * Gyroscope offsets are automatically subtracted.
     *
     * @return Pointer to sensors_event_t structure with gyroscope data
     */
    sensors_event_t* getGyroEvent();

    /**
     * @brief Get fused orientation angles from complementary filter
     *
     * Combines accelerometer and gyroscope data for accurate orientation.
     *
     * @param pitch Output pitch angle in degrees (forward/backward tilt)
     * @param roll Output roll angle in degrees (left/right tilt)
     * @param yaw Output yaw angle in degrees (heading - may drift)
     */
    void getFusedOrientation(float& pitch, float& roll, float& yaw);

    /**
     * @brief Update sensor readings and fusion filter
     *
     * This should be called at regular intervals (50Hz recommended).
     * Reads both accelerometer and gyroscope, applies calibration,
     * and updates the complementary filter for orientation estimation.
     */
    void update();

    /**
     * @brief Check if the sensor is initialized
     *
     * @return true if sensor is initialized and ready
     */
    bool isInitialized() const override { return sensorInitialized; }
};
