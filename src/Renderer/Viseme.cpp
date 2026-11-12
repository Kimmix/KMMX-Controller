#include "Viseme.h"
#include <cmath>

bool Viseme::initMic() {
    if (!mic.init(i2sSampleRate, i2sHopSamples)) { core.initFailed(); return false; }
    if (dsps_fft2r_init_fc32(nullptr, i2sSamples) != ESP_OK) {
        mic.stop_i2s(); core.initFailed(); return false;
    }
    for (int i = 0; i < i2sSamples; ++i)
        hannWindow[i] = 0.5f * (1 - cosf(2 * PI * i / (i2sSamples - 1)));
    return true;
}

void Viseme::taskFailed() {
    mic.stop_i2s();
    dsps_fft2r_deinit_fc32();
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.initFailed();
    xSemaphoreGive(mutex);
}

Viseme::Snapshot Viseme::snapshot() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    Snapshot result{core.settings, core.status};
    xSemaphoreGive(mutex);
    return result;
}
std::array<uint8_t, LipSync::ScalesSize> Viseme::readScales() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const auto scales = core.readScales();
    xSemaphoreGive(mutex);
    return scales;
}
void Viseme::writeScales(const uint8_t* bytes, size_t size) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.writeScales(bytes, size);
    xSemaphoreGive(mutex);
}
void Viseme::readSettings(uint8_t* bytes) { LipSync::encodeSettings(snapshot().settings, bytes); }
void Viseme::readStatus(uint8_t* bytes) { LipSync::encodeStatus(snapshot().status, bytes); }
void Viseme::writeSettings(const uint8_t* bytes, size_t size) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.accept(bytes, size);
    xSemaphoreGive(mutex);
}
void Viseme::calibrationCommand(const uint8_t* bytes, size_t size) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.command(size == 1 ? bytes[0] : 0, millis());
    xSemaphoreGive(mutex);
}
const uint8_t* Viseme::renderViseme(bool talking) {
    const bool validAudio = mic.read(i2sBuffer, i2sHopSamples);
    const bool ready = audioWindow.append(i2sBuffer, validAudio ? i2sHopSamples : 0);
    if (validAudio && !ready) return mouthDefault1;
    float rms = 0, scores[5] = {};
    bool clipped = false, nonzero = false;
    if (validAudio) {
        float sum = 0;
        for (float sample : audioWindow.samples) {
            sum += sample; nonzero |= sample != 0;
            clipped |= sample >= 32760 || sample <= -32760;
        }
        const float mean = float(sum) / i2sSamples;
        float squares = 0;
        for (int i = 0; i < i2sSamples; ++i) {
            const float sample = audioWindow.samples[i] - mean;
            squares += sample * sample;
            fftBuffer[2 * i] = sample * hannWindow[i];
            fftBuffer[2 * i + 1] = 0;
        }
        rms = sqrtf(squares / i2sSamples);
        const auto state = snapshot().status.calibration;
        if (talking && state != LipSync::Calibration::Countdown && state != LipSync::Calibration::Measuring) {
            dsps_fft2r_fc32(fftBuffer, i2sSamples);
            dsps_bit_rev_fc32(fftBuffer, i2sSamples);
            // Magnitudes overwrite the buffer in place: index i is written after 2i and 2i+1 are read.
            for (int i = 0; i < i2sSamples / 2; ++i)
                fftBuffer[i] = sqrtf(fftBuffer[2 * i] * fftBuffer[2 * i] + fftBuffer[2 * i + 1] * fftBuffer[2 * i + 1]);
            LipSync::formantScores(fftBuffer, scores);
        }
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.frame(rms, validAudio, clipped, nonzero, talking, scores, millis());
    const auto status = core.status;
    xSemaphoreGive(mutex);
    return status.opening ? visemeFrames[status.shape][status.opening - 1] : mouthDefault1;
}
