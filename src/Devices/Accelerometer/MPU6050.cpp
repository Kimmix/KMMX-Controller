#include "MPU6050.h"

MPU6050::MPU6050() : mpu(Adafruit_MPU6050()) {}

bool MPU6050::setUp() {
    // Try to initialize MPU6050 at default I2C address 0x68
    sensorInitialized = mpu.begin(0x68);
    
    if (sensorInitialized) {
        // Configure accelerometer range to ±2G (matches LIS3DH for consistent motion detection)
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        
        // Configure gyroscope range (not used, but set to minimum for lower noise)
        mpu.setGyroRange(MPU6050_RANGE_250_DEG);
        
        // Set filter bandwidth to ~50Hz equivalent
        // MPU6050_BAND_21_HZ provides good balance between noise reduction and responsiveness
        // Actual output rate will be ~50Hz which matches the sensor polling rate
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        
        Serial.println("MPU6050 configured: ±2G range, ~50Hz effective rate");
    }
    
    return sensorInitialized;
}

void MPU6050::setDataRate(uint8_t rate) {
    // Note: MPU6050 uses filter bandwidth instead of explicit data rate
    // The rate parameter is kept for interface compatibility but not currently used
    // Data rate is effectively controlled by the filter bandwidth setting in setUp()
    if (sensorInitialized) {
        // Could add rate mapping here if needed in the future
        // For now, we use the fixed configuration from setUp()
    }
}

sensors_event_t* MPU6050::getSensorEvent() {
    if (sensorInitialized) {
        // Read sensor data (updates accelEvent, gyroEvent, tempEvent)
        mpu.getEvent(&accelEvent, &gyroEvent, &tempEvent);
    } else {
        // Return safe default values if sensor not initialized
        // Simulate normal gravity when upright (same as LIS3DH default)
        accelEvent.acceleration.x = 0.0f;
        accelEvent.acceleration.y = -9.8f;  // Default gravity when upright
        accelEvent.acceleration.z = 0.0f;
    }
    
    return &accelEvent;
}
