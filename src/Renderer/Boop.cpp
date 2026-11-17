#include "Boop.h"

float Boop::speedFor(unsigned long elapsed) {
    if (elapsed <= 100) return 1.0f;
    if (elapsed >= boopMaxDuration) return 0.0f;
    return 1.0f - static_cast<float>(elapsed - 100) / (boopMaxDuration - 100);
}

BoopResult Boop::update(uint16_t proximity, unsigned long now) {
    switch (state) {
        case State::IDLE:
            if (proximity >= boopMaxThreshold) {
                state = State::HELD;
                return {BoopEvent::COMPLETED, 1.0f};
            }
            if (proximity > boopMinThreshold) {
                state = State::APPROACHING;
                approachStartedAt = now;
                return {BoopEvent::APPROACHING};
            }
            return {BoopEvent::IDLE};

        case State::APPROACHING:
            if (proximity >= boopMaxThreshold) {
                state = State::HELD;
                return {BoopEvent::COMPLETED, speedFor(now - approachStartedAt)};
            }
            if (proximity <= boopMinThreshold) {
                state = State::IDLE;
                return {BoopEvent::INCOMPLETE_RELEASE};
            }
            return {BoopEvent::APPROACHING};

        case State::HELD:
            if (proximity < boopMaxThreshold) {
                state = State::RELEASING;
                return {BoopEvent::IDLE};
            }
            return {BoopEvent::HELD};

        case State::RELEASING:
            if (proximity >= boopMaxThreshold) {
                state = State::HELD;
                return {BoopEvent::COMPLETED, 1.0f};
            }
            if (proximity <= boopMinThreshold) {
                state = State::IDLE;
                return {BoopEvent::RELEASED};
            }
            return {BoopEvent::IDLE};

    }

    return {BoopEvent::IDLE};
}
