#pragma once
#include "esp_dsp.h"
#include "Utils/Utils.h"
#include "VisemeConfig.h"
#include "VisemeCore.h"
#include "Devices/Microphone/I2SMicrophone.h"
#include "Bitmaps/Bitmaps.h"
#include <freertos/semphr.h>

class Viseme {
public:
    enum VisemeType { AH, EE, OH, OO, TH };
    bool initMic();
    void taskFailed();
    const uint8_t* renderViseme(bool talking);
    const uint8_t** getFrames(VisemeType type) { return visemeFrames[type]; }
    struct Snapshot { LipSync::Settings settings; LipSync::Status status; };
    Snapshot snapshot();
    std::array<uint8_t, 5> readScales();
    void writeScales(const uint8_t* bytes, size_t size);
    void readSettings(uint8_t* bytes);
    void readStatus(uint8_t* bytes);
    void writeSettings(const uint8_t* bytes, size_t size);
    void calibrationCommand(const uint8_t* bytes, size_t size);
private:
    I2SMicrophone mic;
    StaticSemaphore_t mutexStorage;
    SemaphoreHandle_t mutex = xSemaphoreCreateMutexStatic(&mutexStorage);
    LipSync::Core core;
    int16_t i2sBuffer[i2sSamples];
    float fftBuffer[2 * i2sSamples];
    float hannWindow[i2sSamples];
    static const uint8_t visemeFramelength = 60;
    const uint8_t* visemeFrames[5][visemeFramelength] = {
        {mouthAH1, mouthAH2, mouthAH3, mouthAH4, mouthAH5, mouthAH6, mouthAH7, mouthAH8, mouthAH9, mouthAH10,
         mouthAH11, mouthAH12, mouthAH13, mouthAH14, mouthAH15, mouthAH16, mouthAH17, mouthAH18, mouthAH19, mouthAH20,
         mouthAH21, mouthAH22, mouthAH23, mouthAH24, mouthAH25, mouthAH26, mouthAH27, mouthAH28, mouthAH29, mouthAH30,
         mouthAH31, mouthAH32, mouthAH33, mouthAH34, mouthAH35, mouthAH36, mouthAH37, mouthAH38, mouthAH39, mouthAH40,
         mouthAH41, mouthAH42, mouthAH43, mouthAH44, mouthAH45, mouthAH46, mouthAH47, mouthAH48, mouthAH49, mouthAH50,
         mouthAH51, mouthAH52, mouthAH53, mouthAH54, mouthAH55, mouthAH56, mouthAH57, mouthAH58, mouthAH59, mouthAH60},
        {mouthEE1, mouthEE2, mouthEE3, mouthEE4, mouthEE5, mouthEE6, mouthEE7, mouthEE8, mouthEE9, mouthEE10,
         mouthEE11, mouthEE12, mouthEE13, mouthEE14, mouthEE15, mouthEE16, mouthEE17, mouthEE18, mouthEE19, mouthEE20,
         mouthEE21, mouthEE22, mouthEE23, mouthEE24, mouthEE25, mouthEE26, mouthEE27, mouthEE28, mouthEE29, mouthEE30,
         mouthEE31, mouthEE32, mouthEE33, mouthEE34, mouthEE35, mouthEE36, mouthEE37, mouthEE38, mouthEE39, mouthEE40,
         mouthEE41, mouthEE42, mouthEE43, mouthEE44, mouthEE45, mouthEE46, mouthEE47, mouthEE48, mouthEE49, mouthEE50,
         mouthEE51, mouthEE52, mouthEE53, mouthEE54, mouthEE55, mouthEE56, mouthEE57, mouthEE58, mouthEE59, mouthEE60},
        {mouthOH1, mouthOH2, mouthOH3, mouthOH4, mouthOH5, mouthOH6, mouthOH7, mouthOH8, mouthOH9, mouthOH10,
         mouthOH11, mouthOH12, mouthOH13, mouthOH14, mouthOH15, mouthOH16, mouthOH17, mouthOH18, mouthOH19, mouthOH20,
         mouthOH21, mouthOH22, mouthOH23, mouthOH24, mouthOH25, mouthOH26, mouthOH27, mouthOH28, mouthOH29, mouthOH30,
         mouthOH31, mouthOH32, mouthOH33, mouthOH34, mouthOH35, mouthOH36, mouthOH37, mouthOH38, mouthOH39, mouthOH40,
         mouthOH41, mouthOH42, mouthOH43, mouthOH44, mouthOH45, mouthOH46, mouthOH47, mouthOH48, mouthOH49, mouthOH50,
         mouthOH51, mouthOH52, mouthOH53, mouthOH54, mouthOH55, mouthOH56, mouthOH57, mouthOH58, mouthOH59, mouthOH60},
        {mouthOO1, mouthOO2, mouthOO3, mouthOO4, mouthOO5, mouthOO6, mouthOO7, mouthOO8, mouthOO9, mouthOO10,
         mouthOO11, mouthOO12, mouthOO13, mouthOO14, mouthOO15, mouthOO16, mouthOO17, mouthOO18, mouthOO19, mouthOO20,
         mouthOO21, mouthOO22, mouthOO23, mouthOO24, mouthOO25, mouthOO26, mouthOO27, mouthOO28, mouthOO29, mouthOO30,
         mouthOO31, mouthOO32, mouthOO33, mouthOO34, mouthOO35, mouthOO36, mouthOO37, mouthOO38, mouthOO39, mouthOO40,
         mouthOO41, mouthOO42, mouthOO43, mouthOO44, mouthOO45, mouthOO46, mouthOO47, mouthOO48, mouthOO49, mouthOO50,
         mouthOO51, mouthOO52, mouthOO53, mouthOO54, mouthOO55, mouthOO56, mouthOO57, mouthOO58, mouthOO59, mouthOO60},
        {mouthTH1, mouthTH2, mouthTH3, mouthTH4, mouthTH5, mouthTH6, mouthTH7, mouthTH8, mouthTH9, mouthTH10,
         mouthTH11, mouthTH12, mouthTH13, mouthTH14, mouthTH15, mouthTH16, mouthTH17, mouthTH18, mouthTH19, mouthTH20,
         mouthTH21, mouthTH22, mouthTH23, mouthTH24, mouthTH25, mouthTH26, mouthTH27, mouthTH28, mouthTH29, mouthTH30,
         mouthTH31, mouthTH32, mouthTH33, mouthTH34, mouthTH35, mouthTH36, mouthTH37, mouthTH38, mouthTH39, mouthTH40,
         mouthTH41, mouthTH42, mouthTH43, mouthTH44, mouthTH45, mouthTH46, mouthTH47, mouthTH48, mouthTH49, mouthTH50,
         mouthTH51, mouthTH52, mouthTH53, mouthTH54, mouthTH55, mouthTH56, mouthTH57, mouthTH58, mouthTH59, mouthTH60}};

};
