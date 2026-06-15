#pragma once

// Thread-safe sensor data structure for double-buffering
struct SensorData {
    // Accelerometer data (m/s²)
    float accelX = 0;
    float accelY = 0;
    float accelZ = 0;
    float accelMagnitude = 0;  // Cached magnitude for performance

    // Gyroscope data (rad/s) - rotation rates
    float gyroX = 0;  // Pitch rate (rotation around X-axis)
    float gyroY = 0;  // Roll rate (rotation around Y-axis)
    float gyroZ = 0;  // Yaw rate (rotation around Z-axis)
    float gyroMagnitude = 0;  // Total rotation rate magnitude

    // Fused orientation data (degrees) - from complementary filter
    float pitch = 0;   // Forward/backward tilt angle
    float roll = 0;    // Left/right tilt angle
    float yaw = 0;     // Heading (may drift without magnetometer)

    // Other sensors
    uint16_t proximity = 0;
};
