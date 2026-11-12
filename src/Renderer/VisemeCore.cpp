#include "VisemeCore.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "VisemeConfig.h"

namespace {
constexpr uint16_t hundredths(float value) { return uint16_t(value * 100 + 0.5f); }
constexpr std::array<uint16_t, 5> defaultScales = {hundredths(visemeAhScale), hundredths(visemeEeScale),
    hundredths(visemeOhScale), hundredths(visemeOoScale), hundredths(visemeThScale)};
}

LipSync::Core::Core() : scales(defaultScales) {}

void LipSync::Core::writeScales(const uint8_t* bytes, size_t size) {
    if (size == 1 && bytes[0] == 0xff) { scales = defaultScales; return; }
    if (size != ScalesSize || bytes[0] != 2) return;
    std::array<uint16_t, 5> next;
    for (size_t i = 0; i < next.size(); ++i) {
        next[i] = uint16_t(bytes[1 + i * 2]) | (uint16_t(bytes[2 + i * 2]) << 8);
        if (next[i] > 500) return;
    }
    scales = next;
}

std::array<uint8_t, LipSync::ScalesSize> LipSync::Core::readScales() const {
    std::array<uint8_t, ScalesSize> bytes{};
    bytes[0] = 2;
    for (size_t i = 0; i < scales.size(); ++i) {
        bytes[1 + i * 2] = scales[i] & 0xff;
        bytes[2 + i * 2] = scales[i] >> 8;
    }
    return bytes;
}

namespace LipSync {
namespace {
void put16(uint8_t* p, uint16_t value) { p[0] = value; p[1] = value >> 8; }
void putFloat(uint8_t* p, float value) {
    static_assert(sizeof(float) == 4, "BLE requires float32");
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    for (int i = 0; i < 4; ++i) p[i] = bits >> (8 * i);
}
float getFloat(const uint8_t* p) {
    uint32_t bits = 0;
    for (int i = 0; i < 4; ++i) bits |= uint32_t(p[i]) << (8 * i);
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}
}
bool operator==(const Settings& a, const Settings& b) {
    return a.threshold == b.threshold && a.gain == b.gain && a.response == b.response;
}
bool valid(const Settings& s) {
    return std::isfinite(s.threshold) && s.threshold >= 1 && s.threshold <= 1000 &&
           std::isfinite(s.gain) && s.gain >= 0.25f && s.gain <= 4 && uint8_t(s.response) <= 2;
}
bool decodeSettings(const uint8_t* bytes, size_t size, Settings& settings) {
    if (size != SettingsSize || !bytes || bytes[0] != 2) return false;
    Settings decoded{getFloat(bytes + 1), getFloat(bytes + 5), Response(bytes[9])};
    if (!valid(decoded)) return false;
    settings = decoded;
    return true;
}
void encodeSettings(const Settings& s, uint8_t* bytes) {
    bytes[0] = 2;
    putFloat(bytes + 1, s.threshold);
    putFloat(bytes + 5, s.gain);
    bytes[9] = uint8_t(s.response);
}
void encodeStatus(const Status& s, uint8_t* bytes) {
    bytes[0] = 3;
    put16(bytes + 1, s.accepted); put16(bytes + 3, s.applied);
    putFloat(bytes + 5, s.rms);
    bytes[9] = s.gate; bytes[10] = s.shape; bytes[11] = s.opening;
    bytes[12] = uint8_t(s.calibration); bytes[13] = s.progress;
    bytes[14] = uint8_t(s.mic); bytes[15] = uint8_t(s.result);
}
bool Core::calibrating() const {
    return status.calibration == Calibration::Countdown || status.calibration == Calibration::Measuring;
}
void Core::change(const Settings& value) {
    if (!(settings == value)) {
        settings = value;
        if (++status.accepted == 0) ++status.accepted;
    }
}
bool Core::accept(const uint8_t* bytes, size_t size) {
    Settings value;
    if (!decodeSettings(bytes, size, value)) { status.result = Result::Invalid; return false; }
    if (calibrating()) { status.result = Result::Busy; return false; }
    status.result = Result::Ok;
    change(value);
    return true;
}
bool Core::command(uint8_t command, uint32_t now) {
    if (command != 1 && command != 2) { status.result = Result::Invalid; return false; }
    if (command == 2) {
        if (calibrating()) status.calibration = Calibration::Cancelled;
        status.result = Result::Ok;
        return true;
    }
    if (calibrating()) { status.result = Result::Busy; return false; }
    if (status.mic != MicState::Ready) { status.result = Result::AudioError; return false; }
    calibrationAt = now; sampleCount = 0; anyNonzero = false;
    status.calibration = Calibration::Countdown; status.progress = 0; status.result = Result::Ok;
    resetAnimation();
    return true;
}
void Core::resetAnimation() {
    opening = 0; status.opening = 0; status.gate = false; status.shape = 0;
    below = false; candidateFrames = 0; appliedThreshold = -1;
}
void Core::failCalibration(Result result) {
    status.calibration = Calibration::Failed;
    status.result = result;
}
void Core::initFailed() {
    resetAnimation(); status.mic = MicState::InitError; status.result = Result::AudioError;
    if (calibrating()) failCalibration(Result::AudioError);
}
void Core::frame(float rms, bool validAudio, bool clipped, bool nonzero,
                 bool talking, const float rawScores[5], uint32_t now) {
    float scores[5] = {};
    if (rawScores)
        for (size_t i = 0; i < scales.size(); ++i) scores[i] = rawScores[i] * (scales[i] / 100.0f);
    bool validScores = true;
    float total = 0;
    for (float score : scores) {
        validScores = validScores && std::isfinite(score) && score >= 0;
        total += score;
    }
    validScores = validScores && std::isfinite(total);
    for (size_t i = 0; i < sizeof(status.scoreWeights); ++i)
        status.scoreWeights[i] = validScores && total > 0 ? uint8_t(std::round(scores[i] / total * 255)) : 0;
    const float dt = hasFrame ? std::min(uint32_t(now - lastFrame), uint32_t(100)) : 32;
    lastFrame = now; hasFrame = true;
    if (status.mic == MicState::InitError) return;
    if (!validAudio || !std::isfinite(rms) || rms < 0) {
        resetAnimation(); status.rms = 0; status.mic = MicState::ReadError;
        if (calibrating()) failCalibration(Result::AudioError);
        return;
    }
    status.mic = MicState::Ready; status.rms = rms;
    if (calibrating()) {
        if (clipped) failCalibration(Result::Clipping);
        else if (uint32_t(now - calibrationAt) < 2000) {
            status.progress = uint32_t(now - calibrationAt) * 100 / 2000;
        } else {
            status.calibration = Calibration::Measuring;
            calibrationSamples[sampleCount++] = rms;
            anyNonzero |= nonzero;
            status.progress = sampleCount * 100 / 64;
            if (sampleCount == 64) {
                std::sort(calibrationSamples, calibrationSamples + 64);
                Settings value = settings;
                value.threshold = std::max(1.0f, 1.5f * calibrationSamples[57] + 2);
                if (!anyNonzero) failCalibration(Result::ZeroAudio);
                else if (!valid(value)) failCalibration(Result::TooLoud);
                else {
                    change(value);
                    status.calibration = Calibration::Success; status.result = Result::Ok;
                }
            }
        }
        resetAnimation(); status.applied = status.accepted;
        return;
    }
    status.applied = status.accepted;
    if (talking != wasTalking) resetAnimation();
    wasTalking = talking;
    if (appliedThreshold != settings.threshold) {
        status.gate = rms >= settings.threshold;
        appliedThreshold = settings.threshold; below = false;
    } else if (!status.gate) { status.gate = rms >= settings.threshold; below = false; }
    else if (rms < settings.threshold * 0.8f) {
        if (!below) { below = true; belowAt = now; }
        if (uint32_t(now - belowAt) >= 80) status.gate = false;
    } else below = false;

    if (!talking) { opening = 0; status.opening = 0; candidateFrames = 0; return; }
    if (status.gate && rawScores && validScores) {
        int winner = 0;
        float second = 0;
        for (int i = 1; i < 5; ++i) {
            if (scores[i] > scores[winner]) { second = scores[winner]; winner = i; }
            else second = std::max(second, scores[i]);
        }
        if (scores[winner] > 0 && scores[winner] >= second * 1.15f && winner != status.shape) {
            if (candidate != winner) { candidate = winner; candidateFrames = 0; }
            if (++candidateFrames >= 2) { status.shape = winner; candidateFrames = 0; }
        } else candidateFrames = 0;
    } else candidateFrames = 0;
    const float target = status.gate ? std::pow(std::min(std::max(rms - settings.threshold, 0.0f) * settings.gain / 80, 1.0f), 0.8f) : 0;
    const float attack[] = {25, 50, 90}, release[] = {70, 120, 200};
    const float tau = target > opening ? attack[uint8_t(settings.response)] : release[uint8_t(settings.response)];
    opening += (target - opening) * (1 - std::exp(-dt / tau));
    if (!status.gate && opening < 1.0f / 120) opening = 0;
    status.opening = uint8_t(std::min(60.0f, std::round(opening * 60)));
}
}
