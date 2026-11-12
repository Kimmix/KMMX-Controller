#include "../src/Devices/Fan/Fan.h"
#include <cassert>
SerialStub Serial;
int duty = -1, writes = 0;
bool ledcAttachChannel(int, int, int, int) { return true; }
void ledcWriteChannel(int, int value) { duty = value; ++writes; }
int main() {
    Fan fan;
    auto settle = [&] { for (int i = 0; i < 1000; ++i) fan.update(); };
    assert(duty == 0);
    fan.setSpeed(100); settle(); assert(duty == 255);
    const int before = writes; settle(); assert(writes == before);
    fan.setSpeed(0); settle(); assert(duty == 0);
    fan.setSpeed(40); settle(); assert(duty == 102);
    fan.setSpeed(100);
    for (int i = 0; i < 3; ++i) fan.update();
    fan.setSpeed(0); settle(); assert(duty == 0);
}
