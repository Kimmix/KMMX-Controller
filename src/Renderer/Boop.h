#pragma once
#include <Arduino.h>
#include "config.h"

enum class BoopEvent {
    IDLE,
    APPROACHING,
    COMPLETED,
    HELD,
    RELEASED,
    INCOMPLETE_RELEASE
};

struct BoopResult {
    BoopEvent event = BoopEvent::IDLE;
    float speed = 0.0f;
};

class Boop {
   private:
    enum class State { IDLE, APPROACHING, HELD, RELEASING };

    State state = State::IDLE;
    unsigned long approachStartedAt = 0;

    static float speedFor(unsigned long elapsed);

   public:
    void reset() { state = State::IDLE; }
    BoopResult update(uint16_t proximity, unsigned long now = millis());
};
