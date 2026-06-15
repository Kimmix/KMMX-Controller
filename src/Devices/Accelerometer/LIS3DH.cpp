#include "LIS3DH.h"

LIS3DH::LIS3DH() : lis(Adafruit_LIS3DH()) {}

bool LIS3DH::setUp() {
    sensorInitialized = lis.begin(0x18);
    if (sensorInitialized) {
        lis.setDataRate(LIS3DH_DATARATE_50_HZ);  // 50Hz matches sensor polling rate
        lis.setRange(LIS3DH_RANGE_2_G);
    }
    return sensorInitialized;
}

sensors_event_t* LIS3DH::getSensorEvent() {
    if (sensorInitialized) {
        lis.getEvent(&event);
    } else {
        // Return safe default values if sensor not initialized
        event.acceleration.x = 0.0f;
        event.acceleration.y = -9.8f;  // Default gravity when upright
        event.acceleration.z = 0.0f;
    }
    return &event;
}
