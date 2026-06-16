#pragma once

#include <map>
#include "esp_dsp.h"
#include "Utils/Utils.h"
#include "VisemeConfig.h"
#include "Devices/Microphone/I2SMicrophone.h"
#include "Bitmaps/Bitmaps.h"

class Viseme {
   public:
    enum VisemeType {
        AH,
        EE,
        OH,
        OO,
        TH
    };

    void initMic();
    const uint8_t* renderViseme();
    float getNoiseThreshold();
    void setNoiseThreshold(float value);
    VisemeType getCurrentViseme() const { return previousViseme; }
    uint16_t getLoudness() const { return currentLoudness; }

   private:
    I2SMicrophone mic;

    // ESP-DSP FFT buffers (using float for better performance)
    int16_t i2sBuffer[i2sSamples];              // Raw I2S samples
    float tempSamples[i2sSamples];              // Smoothed + windowed samples
    float fftBuffer[2 * i2sSamples];            // Interleaved complex: [re0,im0,re1,im1,...]
    float hannWindow[i2sSamples];               // Pre-computed Hann window
    float magnitudes[i2sSamples / 2];           // Magnitude spectrum

    // Viseme amplitudes
    float ahAmplitude = 0, eeAmplitude = 0, ohAmplitude = 0;
    float ooAmplitude = 0, thAmplitude = 0;

    float noiseThreshold = visemeNoiseThreshold;
    const float alpha = visemeSmoothingAlpha;
    const float decayRate = visemeDecayRate;
    uint16_t currentLoudness = 0;
    unsigned long decayStartTime = 0;
    const uint16_t decayElapsedThreshold = 1000;
    static const uint8_t visemeFramelength = 20;
    VisemeType previousViseme;

    const uint8_t* ahViseme[visemeFramelength] = {
        mouthAH1, mouthAH2, mouthAH3, mouthAH4, mouthAH5, mouthAH6, mouthAH7, mouthAH8, mouthAH9, mouthAH10,
        mouthAH11, mouthAH12, mouthAH13, mouthAH14, mouthAH15, mouthAH16, mouthAH17, mouthAH18, mouthAH19, mouthAH20};
    const uint8_t* eeViseme[visemeFramelength] = {
        mouthEE1, mouthEE2, mouthEE3, mouthEE4, mouthEE5, mouthEE6, mouthEE7, mouthEE8, mouthEE9, mouthEE10,
        mouthEE11, mouthEE12, mouthEE13, mouthEE14, mouthEE15, mouthEE16, mouthEE17, mouthEE18, mouthEE19, mouthEE20};
    const uint8_t* ohViseme[visemeFramelength] = {
        mouthOH1, mouthOH2, mouthOH3, mouthOH4, mouthOH5, mouthOH6, mouthOH7, mouthOH8, mouthOH9, mouthOH10,
        mouthOH11, mouthOH12, mouthOH13, mouthOH14, mouthOH15, mouthOH16, mouthOH17, mouthOH18, mouthOH19, mouthOH20};
    const uint8_t* ooViseme[visemeFramelength] = {
        mouthOO1, mouthOO2, mouthOO3, mouthOO4, mouthOO5, mouthOO6, mouthOO7, mouthOO8, mouthOO9, mouthOO10,
        mouthOO11, mouthOO12, mouthOO13, mouthOO14, mouthOO15, mouthOO16, mouthOO17, mouthOO18, mouthOO19, mouthOO20};
    const uint8_t* thViseme[visemeFramelength] = {
        mouthTH1, mouthTH2, mouthTH3, mouthTH4, mouthTH5, mouthTH6, mouthTH7, mouthTH8, mouthTH9, mouthTH10,
        mouthTH11, mouthTH12, mouthTH13, mouthTH14, mouthTH15, mouthTH16, mouthTH17, mouthTH18, mouthTH19, mouthTH20};

    // Initialization
    void initHannWindow();
    bool initFFT();

    // Audio processing pipeline
    void readAndPrepareSamples();
    void performFFT();
    void calculateMagnitudes();
    void analyzeVisemes();

    // Helper functions
    inline float binToFrequency(int bin);
    void calculateAmplitude(float ah, float ee, float oh, float oo, float th, float& minAmp, float& maxAmp, float& avgAmp);
    void normalizeViseme(float& ah_amplitude, float& ee_amplitude, float& oh_amplitude, float& oo_amplitude, float& th_amplitude);
    void levelBoost(VisemeType viseme, float& maxAmp);
    unsigned int calculateLoudness(float max, float avg);
    unsigned int smoothedLoudness(unsigned int input);
    VisemeType getDominantViseme(float& maxAmp);
    VisemeType holdViseme(VisemeType input);
    const uint8_t* visemeOutput(VisemeType viseme, unsigned int level);
};
