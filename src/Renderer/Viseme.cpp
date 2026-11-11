#include "Viseme.h"
#include <cmath>

bool Viseme::initMic() {
    if (!mic.init(i2sSampleRate, i2sSamples)) { core.initFailed(); return false; }
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
std::array<uint8_t, 5> Viseme::readScales() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    const auto scales = core.scales;
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
    const bool validAudio = mic.read(i2sBuffer, i2sSamples);
    float rms = 0, scores[5] = {};
    bool clipped = false, nonzero = false;
    if (validAudio) {
        int32_t sum = 0;
        for (int16_t sample : i2sBuffer) {
            sum += sample; nonzero |= sample != 0;
            clipped |= sample >= 32760 || sample <= -32760;
        }
        const float mean = float(sum) / i2sSamples;
        float squares = 0;
        for (int i = 0; i < i2sSamples; ++i) {
            const float sample = i2sBuffer[i] - mean;
            squares += sample * sample;
            fftBuffer[2 * i] = sample * hannWindow[i];
            fftBuffer[2 * i + 1] = 0;
        }
        rms = sqrtf(squares / i2sSamples);
        const auto state = snapshot().status.calibration;
        if (talking && state != LipSync::Calibration::Countdown && state != LipSync::Calibration::Measuring) {
            dsps_fft2r_fc32(fftBuffer, i2sSamples);
            dsps_bit_rev_fc32(fftBuffer, i2sSamples);
            const uint16_t low[] = {visemeAhFreqMin, visemeEeFreqMin, visemeOhFreqMin, visemeOoFreqMin, visemeThFreqMin};
            const uint16_t high[] = {visemeAhFreqMax, visemeEeFreqMax, visemeOhFreqMax, visemeOoFreqMax, visemeThFreqMax};
            for (int i = 0; i < i2sSamples / 2; ++i) {
                float frequency = i * i2sSampleRate / i2sSamples;
                float magnitude = hypotf(fftBuffer[2 * i], fftBuffer[2 * i + 1]);
                for (int band = 0; band < 5; ++band)
                    if (frequency >= low[band] && frequency <= high[band]) scores[band] += magnitude;
            }
        }
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    core.frame(rms, validAudio, clipped, nonzero, talking, scores, millis());
    const auto status = core.status;
    xSemaphoreGive(mutex);
    return status.opening ? visemeFrames[status.shape][status.opening - 1] : mouthDefault1;
}
