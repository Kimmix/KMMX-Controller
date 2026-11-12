#include "../src/Renderer/VisemeCore.h"
#include <cassert>
#include <cmath>
#include "../src/Renderer/AudioWindow.h"
#include <cstring>
#include <limits>

// Hann-windowed magnitude spectrum of a synthetic frame, as the firmware FFT produces.
static void spectrum(const float* samples, float* magnitude) {
    const float pi = 3.14159265f;
    for (int k = 0; k < i2sSamples / 2; ++k) {
        float re = 0, im = 0;
        for (int n = 0; n < i2sSamples; ++n) {
            const float s = samples[n] * 0.5f * (1 - std::cos(2 * pi * n / (i2sSamples - 1)));
            re += s * std::cos(2 * pi * k * n / i2sSamples);
            im -= s * std::sin(2 * pi * k * n / i2sSamples);
        }
        magnitude[k] = std::sqrt(re * re + im * im);
    }
}
static int winner(const float* scores) {
    int best = 0;
    for (int i = 1; i < 5; ++i) if (scores[i] > scores[best]) best = i;
    return best;
}
static void testFormants() {
    const float pi = 3.14159265f;
    // F1, F2, F3 for "ah", "ee", "oh", "oo" in a male and a female voice (Peterson & Barney).
    const float voices[2][4][3] = {
        {{730, 1090, 2440}, {270, 2290, 3010}, {570, 840, 2410}, {300, 870, 2240}},
        {{850, 1220, 2810}, {310, 2790, 3310}, {590, 920, 2710}, {370, 950, 2670}}};
    const float pitches[2][2] = {{100, 140}, {200, 250}};
    float samples[i2sSamples], magnitude[i2sSamples / 2], scores[5];
    for (int voice = 0; voice < 2; ++voice) for (float pitch : pitches[voice]) for (int vowel = 0; vowel < 4; ++vowel) {
        std::fill(samples, samples + i2sSamples, 0.0f);
        for (int h = 1; h * pitch < 7000; ++h) {
            const float f = h * pitch;
            float gain = 1.0f / h; // glottal source plus lip radiation: -6 dB/octave
            for (int j = 0; j < 3; ++j) { // second-order resonator per formant
                const float F = voices[voice][vowel][j], B = 60 + 40 * j;
                gain *= F * F / std::sqrt((F * F - f * f) * (F * F - f * f) + B * B * f * f);
            }
            for (int n = 0; n < i2sSamples; ++n) samples[n] += gain * std::sin(2 * pi * f * n / i2sSampleRate);
        }
        spectrum(samples, magnitude);
        LipSync::formantScores(magnitude, scores);
        assert(winner(scores) == vowel);
    }
    uint32_t seed = 1;
    float previous = 0;
    for (float& sample : samples) { // differenced white noise: energy rises with frequency like "s"
        seed = seed * 1664525u + 1013904223u;
        const float noise = float(seed >> 8) / (1 << 24) - 0.5f;
        sample = noise - previous; previous = noise;
    }
    spectrum(samples, magnitude);
    LipSync::formantScores(magnitude, scores);
    assert(winner(scores) == 4);
    std::fill(magnitude, magnitude + i2sSamples / 2, 0.0f);
    LipSync::formantScores(magnitude, scores);
    for (float score : scores) assert(score == 0);
}

int main() {
    testFormants();
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
    const float winner[] = {0, 100, 0, 0, 0};
    for (uint32_t step : {16u, 32u}) {
        LipSync::Core timed;
        timed.frame(100, true, false, true, true, winner, 100);
        if (step == 16) {
            timed.frame(100, true, false, true, true, winner, 116);
            assert(timed.status.shape == 0);
        }
        timed.frame(100, true, false, true, true, winner, 132);
        assert(timed.status.shape == 1);
    }
    assert(LipSync::sampleAmplitude(2146959360) == 32760);
    assert(LipSync::sampleAmplitude(-2146959360) == -32760);
    LipSync::Core interrupted;
    const float noWinner[5] = {};
    interrupted.frame(100, true, false, true, true, winner, 100);
    interrupted.frame(100, true, false, true, true, noWinner, 116);
    interrupted.frame(100, true, false, true, true, winner, 132);
    assert(interrupted.status.shape == 0);
    interrupted.frame(0, false, false, false, true, nullptr, 148);
    interrupted.frame(100, true, false, true, true, winner, 164);
    assert(interrupted.status.shape == 0);
    interrupted.frame(100, true, false, true, true, winner, 196);
    assert(interrupted.status.shape == 1);
    LipSync::Core calibration;
    assert(calibration.command(1, 0));
    for (uint32_t now = 0; now < 4016; now += 16)
        calibration.frame(10, true, false, true, true, winner, now);
    assert(calibration.status.calibration == LipSync::Calibration::Measuring);
    calibration.frame(10, true, false, true, true, winner, 4016);
    assert(calibration.status.calibration == LipSync::Calibration::Success);

    LipSync::Core core;
    const float scores[] = {10, 0, 20, 0, 0};
    core.frame(100, true, false, true, true, scores, 100);
    const uint8_t expected[] = {85, 0, 170, 0, 0};
    assert(std::memcmp(core.status.scoreWeights, expected, sizeof(expected)) == 0);

    const auto defaults = core.scales;
    assert((defaults == std::array<uint16_t, 5>{100, 100, 100, 100, 100}));
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
    core.frame(100, true, false, true, true, scores, 132);
    core.frame(100, true, false, true, true, scores, 164);
    assert(core.status.shape == 2 && core.status.scoreWeights[2] == 255);
    const uint8_t zeros[11] = {2};
    core.writeScales(zeros, sizeof(zeros));
    core.frame(100, true, false, true, true, scores, 196);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);
    const uint8_t reset = 0xff;
    core.writeScales(&reset, 1);
    assert(core.scales == defaults);

    const float silence[5] = {};
    core.frame(0, true, false, false, true, silence, 228);
    for (auto weight : core.status.scoreWeights) assert(weight == 0);

    for (float bad : {-1.0f, std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max() / 4}) {
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
