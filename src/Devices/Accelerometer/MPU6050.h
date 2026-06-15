#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "IAccelerometer.h"

/**
 * @brief MPU6050 Accelerometer implementation for KimmixControllerV4
 * 
 * This class wraps the Adafruit MPU6050 library to provide accelerometer
 * functionality compatible with the IAccelerometer interface.
 * 
 * Hardware Details:
 * - I2C Address: 0x68 (default)
 * - Data Rate: ~50Hz (matches sensor polling rate)
 * - Range: ±2G (sufficient for motion detection)
 * 
 * The MPU6050 is a 6-axis IMU (accelerometer + gyroscope), but this
 * implementation only uses the accelerometer portion for motion detection.
 */
class MPU6050 : public IAccelerometer {
private:
    sensors_event_t accelEvent;     // Cached acceleration event
    sensors_event_t gyroEvent;      // Gyro event (unused, but required by API)
    sensors_event_t tempEvent;      // Temperature event (unused, but required by API)
    Adafruit_MPU6050 mpu;           // MPU6050 driver instance
    bool sensorInitialized = false; // Initialization status

public:
    /**
     * @brief Construct a new MPU6050 accelerometer object
     */
    MPU6050();

    /**
     * @brief Initialize the MPU6050 sensor
     * 
     * Configures the sensor with:
     * - I2C address 0x68
     * - Data rate ~50Hz
     * - Accelerometer range ±2G
     * - Gyroscope disabled (not used)
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
     * @brief Set the sensor data rate
     * 
     * Note: MPU6050 data rate configuration is different from LIS3DH.
     * This method accepts a simplified rate parameter and maps it to
     * appropriate MPU6050 filter bandwidth settings.
     * 
     * @param rate Data rate value (currently unused - fixed at ~50Hz)
     */
    void setDataRate(uint8_t rate) override;

    /**
     * @brief Check if the sensor is initialized
     * 
     * @return true if sensor is initialized and ready
     */
    bool isInitialized() const override { return sensorInitialized; }
};
