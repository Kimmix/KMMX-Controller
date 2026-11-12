#include "../src/Renderer/VisemeCore.h"
#include <cassert>
#include <cstring>
#include <limits>

int main() {
    LipSync::Core core;
    const float scores[] = {10, 0, 20, 0, 0};
    core.frame(20, true, false, true, true, scores, 100);
    const uint8_t expected[] = {137, 0, 118, 0, 0};
    assert(std::memcmp(core.status.scoreWeights, expected, sizeof(expected)) == 0);

    const auto defaults = core.scales;
    assert((defaults == std::array<uint8_t, 5>{35, 13, 15, 11, 10}));
    const uint8_t tuned[] = {0, 0, 50, 0, 0};
    core.writeScales(tuned, 5);
    const uint8_t invalid[] = {1, 2, 3, 4, 51};
    core.writeScales(invalid, 5);
    core.writeScales(invalid, 4);
    core.writeScales(invalid, 1);
    assert(std::memcmp(core.scales.data(), tuned, 5) == 0);
    core.frame(20, true, false, true, true, scores, 132);
    core.frame(20, true, false, true, true, scores, 164);
    assert(core.status.shape == 2 && core.status.scoreWeights[2] == 255);
    const uint8_t zeros[5] = {};
    core.writeScales(zeros, 5);
    core.frame(20, true, false, true, true, scores, 196);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);
    const uint8_t reset = 0xff;
    core.writeScales(&reset, 1);
    assert(core.scales == defaults);

    const float silence[5] = {};
    core.frame(0, true, false, false, true, silence, 228);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);

    for (float bad : {-1.0f, std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max() / 8}) {
        LipSync::Core checked;
        const float valid[] = {0, 100, 0, 0, 0};
        checked.frame(100, true, false, true, true, valid, 100);
        const float invalid[] = {bad, bad, bad, bad, bad};
        checked.frame(100, true, false, true, true, invalid, 132);
        for (auto weight : checked.status.scoreWeights) assert(weight == 0);
        assert(checked.status.rms == 100 && checked.status.opening > 0);
        assert(checked.status.shape == 0);
        checked.frame(100, true, false, true, true, valid, 164);
        assert(checked.status.shape == 0);
        checked.frame(100, true, false, true, true, valid, 196);
        assert(checked.status.shape == 1 && checked.status.scoreWeights[1] == 255);
    }
}
