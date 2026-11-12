#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "VisemeConfig.h"

namespace LipSync {
// MS4030 carries signed 24-bit audio in the upper bits of a 32-bit slot.
inline float sampleAmplitude(int32_t word) {
    return float(word >> 8) / 256.0f;
}

class AudioWindow {
public:
    float samples[i2sSamples] = {};
    bool append(const int32_t* block, size_t count) {
        if (count != i2sHopSamples) { retained = false; return false; }
        if (retained)
            std::copy(samples + i2sHopSamples, samples + i2sSamples, samples);
        for (size_t i = 0; i < count; ++i)
            samples[i2sHopSamples + i] = sampleAmplitude(block[i]);
        const bool ready = retained;
        retained = true;
        return ready;
    }
private:
    bool retained = false;
};
}
