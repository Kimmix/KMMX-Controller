#pragma once
#include <Arduino.h>
#include "config.h"

/**
 * Boop state machine states.
 *
 * State transitions based on normalized sensor values (0-1023) and
 * thresholds defined in config.h:
 *
 * IDLE              -> No object or value <= boopMinThreshold
 * BOOP_IN_PROGRESS  -> boopMinThreshold < value < boopMaxThreshold
 * BOOP_CONTINUOUS   -> value >= boopMaxThreshold
 * ANGRY             -> value >= 1023 (maximum sensor output)
 */
enum BoopState {
    IDLE,
    BOOP_IN_PROGRESS,
    BOOP_CONTINUOUS,
    ANGRY
};

/**
 * Boop status structure consolidating all output states.
 * Returned by getBoop() to simplify the interface.
 */
struct BoopStatus {
    bool isInRange = false;        // Object in boop range (boopMinThreshold < value < boopMaxThreshold)
    bool isBoop = false;           // Boop completed (reached boopMaxThreshold)
    bool isContinuous = false;     // Continuous boop active (holding at boopMaxThreshold)
    bool isAngry = false;          // Angry state (value >= 1023, too close)
    float boopSpeed = 0.0f;        // Approach speed (0.0-1.0) for animation intensity
};

/**
 * Boop detection and state management.
 *
 * Processes normalized proximity sensor data (0-1023) and manages state
 * transitions. Tracks approach speed based on time elapsed between
 * boopMinThreshold and boopMaxThreshold (see config.h).
 *
 * All sensors must output 0-1023 range (0=far, 1023=touching).
 */
class Boop {
   private:
    BoopState currentBoopState = IDLE;
    unsigned long boopStartTime = 0;

    /** Calculate boop speed from elapsed time (faster = higher 0.0-1.0) */
    float calculateBoopSpeed();

   public:
    /**
     * Process sensor value and update boop state.
     *
     * @param sensorValue Normalized proximity (0-1023)
     * @return BoopStatus structure containing all boop state flags
     */
    BoopStatus getBoop(uint16_t sensorValue);
};
