#include "../src/Renderer/VisemeCore.h"
#include <cassert>
#include <cstring>

int main() {
    LipSync::Core core;
    const float scores[] = {35, 0, 30, 0, 0};
    core.frame(20, true, false, true, true, scores, 100);
    const uint8_t expected[] = {137, 0, 118, 0, 0};
    assert(std::memcmp(core.status.scoreWeights, expected, sizeof(expected)) == 0);

    const float silence[5] = {};
    core.frame(0, true, false, false, true, silence, 120);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);
}
