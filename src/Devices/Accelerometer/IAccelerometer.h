#pragma once

#include <Adafruit_Sensor.h>

/**
 * @brief Abstract interface for accelerometer sensors
 *
 * This interface provides a common API for different accelerometer hardware:
 * - LIS3DH (used in KimmixControllerV2)
 * - MPU6050 (used in KimmixControllerV4)
 *
 * Both implementations provide 3-axis acceleration data via the Adafruit
 * Unified Sensor Library's sensors_event_t structure.
 */
class IAccelerometer {
public:
    virtual ~IAccelerometer() = default;

    /**
     * @brief Initialize the accelerometer sensor
     *
     * Performs hardware initialization, sets up I2C communication,
     * and configures the sensor with appropriate data rate and range.
     *
     * @return true if initialization was successful, false otherwise
     */
    virtual bool setUp() = 0;

    /**
     * @brief Get the latest sensor reading
     *
     * Returns a pointer to a sensors_event_t structure containing the
     * current acceleration data on all three axes (x, y, z) in m/s².
     *
     * If the sensor is not initialized, implementations should return
     * safe default values (typically 0, -9.8, 0 representing normal gravity).
     *
     * @return Pointer to sensors_event_t structure with acceleration data
     */
    virtual sensors_event_t* getSensorEvent() = 0;

    /**
     * @brief Check if the sensor is initialized
     *
     * @return true if the sensor is initialized and ready, false otherwise
     */
    virtual bool isInitialized() const = 0;
};
