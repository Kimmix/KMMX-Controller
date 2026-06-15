#pragma once

#include <Arduino.h>
#include "config.h"

#if HAS_FAN_CONTROL

/**
 * @brief Fan Controller
 *
 * This class manages a 4-pin PWM-controlled fan with tachometer feedback.
 * The fan is controlled via a TXU0202DCUR level shifter chip.
 *
 * Hardware Details:
 * - PWM Pin: IO40 (controls fan speed)
 * - Tachometer Pin: IO41 (reads fan RPM)
 * - PWM Frequency: 25kHz (standard for PC fans)
 * - PWM Resolution: 8-bit (0-255)
 * - PWM Channel: 1 (horn uses channel 0)
 *
 * Features:
 * - Smooth speed transitions with configurable fade
 * - RPM monitoring via interrupt-based tachometer
 * - Enable/disable control
 * - Safe defaults (fan off on startup)
 */
class Fan {
public:
    /**
     * @brief Construct a new Fan controller
     */
    Fan();

    /**
     * @brief Set fan speed with optional fade transition
     *
     * @param speed Target speed (0-100, where 0 is off, 100 is full speed)
     * @param fadeSpeed Transition speed (1-100, higher is faster)
     */
    void setSpeed(int speed, int fadeSpeed = 10);

    /**
     * @brief Get the current fan speed
     *
     * @return Current speed (0-100)
     */
    int getSpeed() const;

    /**
     * @brief Enable or disable the fan
     *
     * When disabled, fan speed is forced to 0.
     *
     * @param enable true to enable, false to disable
     */
    void setEnabled(bool enable);

    /**
     * @brief Check if the fan is enabled
     *
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;

    /**
     * @brief Get the current RPM reading from tachometer
     *
     * @return Fan speed in RPM (0 if fan is off or no signal)
     */
    uint16_t getRPM() const;

    /**
     * @brief Check if fan is connected and responding
     *
     * Detects if a fan is physically connected by checking for tachometer pulses
     * when fan speed is above 0. If no pulses are detected after a timeout,
     * the fan is considered disconnected.
     *
     * @return true if fan is connected and responding, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Update fan state (call regularly from main loop or task)
     *
     * Handles smooth speed transitions and RPM calculation.
     * Should be called at ~50Hz for smooth operation.
     */
    void update();

private:
    // Speed control
    int currentSpeed = 0;      // Current speed (0-100)
    int targetSpeed = 0;       // Target speed (0-100)
    int transitionSpeed = 10;  // Fade speed per update
    float pwmValue = 0.0f;     // Actual PWM duty cycle (for smooth transitions)
    bool enabled = true;       // Enable/disable flag

    // RPM monitoring
    volatile uint32_t pulseCount = 0;     // Pulse counter (modified by ISR)
    uint32_t lastPulseCount = 0;          // Previous pulse count
    uint32_t lastRpmUpdate = 0;           // Last RPM calculation time
    uint16_t currentRPM = 0;              // Current RPM value
    static constexpr uint16_t RPM_UPDATE_INTERVAL = 1000;  // Update RPM every 1 second

    // Connection detection
    bool fanConnected = false;            // Fan connection status
    uint32_t lastConnectionCheck = 0;     // Last time connection was checked
    static constexpr uint16_t CONNECTION_CHECK_INTERVAL = 3000;  // Check every 3 seconds
    static constexpr uint16_t MIN_RPM_THRESHOLD = 100;  // Minimum RPM to consider fan connected

    // Tachometer ISR
    static void IRAM_ATTR tachometerISR();
    static Fan* instance;  // Singleton for ISR access

    /**
     * @brief Calculate RPM from pulse count
     *
     * Called periodically to update RPM reading based on pulse count.
     */
    void calculateRPM();
};

#endif // HAS_FAN_CONTROL
