#include "../src/Renderer/Boop.h"
#include <cassert>
unsigned long millis() { return 0; }
int main() {
    Boop boop;
    auto step = [&](uint16_t proximity, unsigned long now) { return boop.update(proximity, now); };

    // approach -> complete -> release
    assert(step(0, 0).event == BoopEvent::IDLE);
    assert(step(500, 100).event == BoopEvent::APPROACHING);
    BoopResult done = step(950, 600);
    assert(done.event == BoopEvent::COMPLETED && done.speed > 0.0f && done.speed < 1.0f);
    assert(step(950, 700).event == BoopEvent::HELD);
    assert(step(500, 800).event == BoopEvent::IDLE);       // releasing
    assert(step(50, 900).event == BoopEvent::RELEASED);

    // incomplete release
    assert(step(500, 1000).event == BoopEvent::APPROACHING);
    assert(step(50, 1100).event == BoopEvent::INCOMPLETE_RELEASE);

    // too close for 10 s, then lock-out
    assert(step(1023, 2000).event == BoopEvent::TOO_CLOSE);
    assert(step(1023, 11999).event == BoopEvent::TOO_CLOSE);
    assert(step(1023, 12000).event == BoopEvent::LOCKED_OUT);
}
