#pragma once

#include <cstddef>
#include <cstdint>

namespace LipSync {
constexpr size_t SettingsSize = 10;
constexpr size_t StatusSize = 16;
enum class Response : uint8_t { Snappy, Natural, Smooth };
enum class Calibration : uint8_t { Idle, Countdown, Measuring, Success, Failed, Cancelled };
enum class MicState : uint8_t { Ready, ReadError, InitError };
enum class Result : uint8_t { Ok, Invalid, Busy, AudioError, Clipping, ZeroAudio, TooLoud };

struct Settings {
    float threshold = 13.0f;
    float gain = 1.0f;
    Response response = Response::Natural;
};
bool operator==(const Settings& a, const Settings& b);
bool valid(const Settings& settings);
bool decodeSettings(const uint8_t* bytes, size_t size, Settings& settings);
void encodeSettings(const Settings& settings, uint8_t* bytes);

struct Status {
    uint16_t accepted = 1, applied = 0;
    float rms = 0;
    bool gate = false;
    uint8_t shape = 0, opening = 0;
    uint8_t scoreWeights[5] = {};
    Calibration calibration = Calibration::Idle;
    uint8_t progress = 0;
    MicState mic = MicState::Ready;
    Result result = Result::Ok;
};
void encodeStatus(const Status& status, uint8_t* bytes);

// Hardware-independent state machine, also used by the host regression tests.
class Core {
public:
    Settings settings;
    Status status;
    bool accept(const uint8_t* bytes, size_t size);
    bool command(uint8_t command, uint32_t now);
    bool calibrating() const;
    void initFailed();
    void frame(float rms, bool validAudio, bool clipped, bool nonzero,
               bool talking, const float scores[5], uint32_t now);
private:
    void change(const Settings& value);
    void resetAnimation();
    void failCalibration(Result result);
    uint32_t calibrationAt = 0, belowAt = 0, lastFrame = 0;
    bool below = false, wasTalking = false, hasFrame = false;
    float opening = 0, appliedThreshold = -1;
    uint8_t candidate = 0, candidateFrames = 0, sampleCount = 0;
    bool anyNonzero = false;
    float calibrationSamples[64] = {};
};
}
