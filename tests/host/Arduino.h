#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
using std::min;
using std::max;
using std::abs;
using std::round;
struct SerialStub { void println(const char*) {} };
extern SerialStub Serial;
bool ledcAttachChannel(int, int, int, int);
void ledcWriteChannel(int, int);
