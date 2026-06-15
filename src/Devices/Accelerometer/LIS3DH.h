#pragma once

#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include "IAccelerometer.h"

/**
 * @brief LIS3DH Accelerometer implementation for KimmixControllerV2
 *
 * This class wraps the Adafruit LIS3DH library to provide accelerometer
 * functionality compatible with the IAccelerometer interface.
 *
 * Hardware Details:
 * - I2C Address: 0x18
 * - Data Rate: 50Hz (matches sensor polling rate)
 * - Range: ±2G (sufficient for motion detection)
 */
class LIS3DH : public IAccelerometer {
   private:
    sensors_event_t event;
    Adafruit_LIS3DH lis;
    bool sensorInitialized = false;

   public:
    LIS3DH();
    bool setUp() override;
    sensors_event_t* getSensorEvent() override;
    void setDataRate(uint8_t rate) override;
    bool isInitialized() const override { return sensorInitialized; }
};
