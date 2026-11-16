#include "../src/Renderer/VisemeCore.h"
#include <cassert>
#include "../src/Renderer/AudioWindow.h"
#include <cstring>
#include <limits>

int main() {
    assert(LipSync::sampleAmplitude(0x7fffff00) > 32767);
    assert(LipSync::sampleAmplitude(INT32_MIN) == -32768);
    assert(LipSync::sampleAmplitude(65536) == 1);
    assert(LipSync::sampleAmplitude(-65536) == -1);
    LipSync::AudioWindow window;
    int32_t block[i2sHopSamples];
    std::fill(block, block + i2sHopSamples, 65536);
    assert(!window.append(block, i2sHopSamples));
    std::fill(block, block + i2sHopSamples, 131072);
    assert(window.append(block, i2sHopSamples));
    assert(window.samples[0] == 1 && window.samples[511] == 2);
    std::fill(block, block + i2sHopSamples, 196608);
    assert(window.append(block, i2sHopSamples));
    assert(window.samples[0] == 2 && window.samples[511] == 3);
    assert(!window.append(block, 17));
    assert(!window.append(block, i2sHopSamples));
    assert(window.append(block, i2sHopSamples));
    assert(window.samples[0] == 3);
    const float loud = 20000; // above the largest valid threshold, so the gate opens whatever the tuned default is
    const float winner[] = {0, 100, 0, 0, 0};
    for (uint32_t step : {16u, 32u}) {
        LipSync::Core timed;
        timed.frame(loud, true, true, winner, 100);
        if (step == 16) {
            timed.frame(loud, true, true, winner, 116);
            assert(timed.status.shape == 0);
        }
        timed.frame(loud, true, true, winner, 132);
        assert(timed.status.shape == 1);
    }
    assert(LipSync::sampleAmplitude(2146959360) == 32760);
    assert(LipSync::sampleAmplitude(-2146959360) == -32760);
    LipSync::Core interrupted;
    const float noWinner[5] = {};
    interrupted.frame(loud, true, true, winner, 100);
    interrupted.frame(loud, true, true, noWinner, 116);
    interrupted.frame(loud, true, true, winner, 132);
    assert(interrupted.status.shape == 0);
    interrupted.frame(0, false, true, nullptr, 148);
    interrupted.frame(loud, true, true, winner, 164);
    assert(interrupted.status.shape == 0);
    interrupted.frame(loud, true, true, winner, 196);
    assert(interrupted.status.shape == 1);
    LipSync::Core steady; // constant loud sound opens wide, then settles at the 30% floor (18/60)
    uint8_t widest = 0, swaps = 0;
    for (uint32_t now = 0; now < 20000; now += 16) {
        const bool other = now >= 1000 && now < 1300; // a different winner while the mouth is wide
        const float ah[] = {100, 0, 0, 0, 0};
        const uint8_t before = steady.status.shape;
        steady.frame(loud, true, true, other ? ah : winner, now);
        // After the onset, a swap only lands once the mouth has dipped to 40% (24/60).
        if (now > 100 && steady.status.shape != before) { assert(steady.status.opening <= 24); ++swaps; }
        if (steady.status.opening > widest) widest = steady.status.opening;
    }
    assert(widest > 40 && steady.status.opening == 18 && swaps == 2);

    LipSync::Core core;
    const auto defaults = core.scales;
    const uint8_t unit[] = {2, 100, 0, 100, 0, 100, 0, 100, 0, 100, 0}; // keep the checks independent of tuned defaults
    core.writeScales(unit, sizeof(unit));
    const float scores[] = {10, 0, 20, 0, 0};
    core.frame(loud, true, true, scores, 100);
    const uint8_t expected[] = {85, 0, 170, 0, 0};
    assert(std::memcmp(core.status.scoreWeights, expected, sizeof(expected)) == 0);

    const uint8_t precise[] = {2, 163, 0, 1, 0, 244, 1, 0, 0, 100, 0};
    core.writeScales(precise, sizeof(precise));
    assert((core.scales == std::array<uint16_t, 5>{163, 1, 500, 0, 100}));
    const auto readback = core.readScales();
    assert(std::memcmp(readback.data(), precise, sizeof(precise)) == 0);
    auto invalid = readback;
    invalid[9] = 245; invalid[10] = 1; // 501: no partial update.
    core.writeScales(invalid.data(), invalid.size());
    invalid = readback; invalid[0] = 3;
    core.writeScales(invalid.data(), invalid.size());
    core.writeScales(precise, 10);
    const uint8_t legacy[] = {0, 0, 50, 0, 0};
    core.writeScales(legacy, sizeof(legacy));
    assert(core.readScales() == readback);
    const uint8_t tuned[] = {2, 0, 0, 0, 0, 244, 1, 0, 0, 0, 0};
    core.writeScales(tuned, sizeof(tuned));
    core.frame(loud, true, true, scores, 132);
    core.frame(loud, true, true, scores, 164);
    assert(core.status.shape == 2 && core.status.scoreWeights[2] == 255);
    const uint8_t zeros[11] = {2};
    core.writeScales(zeros, sizeof(zeros));
    core.frame(loud, true, true, scores, 196);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);
    const uint8_t reset = 0xff;
    core.writeScales(&reset, 1);
    assert(core.scales == defaults);

    const float silence[5] = {};
    core.frame(0, true, true, silence, 228);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);

    for (float bad : {-1.0f, std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max() / 8}) {
        LipSync::Core checked;
        const float valid[] = {0, 100, 0, 0, 0};
        checked.frame(loud, true, true, valid, 100);
        const float invalid[] = {bad, bad, bad, bad, bad};
        checked.frame(loud, true, true, invalid, 132);
        for (auto weight : checked.status.scoreWeights) assert(weight == 0);
        assert(checked.status.rms == loud && checked.status.opening > 0);
        assert(checked.status.shape == 0);
        checked.frame(loud, true, true, valid, 164);
        assert(checked.status.shape == 0);
        checked.frame(loud, true, true, valid, 196);
        assert(checked.status.shape == 1 && checked.status.scoreWeights[1] == 255);
    }
}
