#pragma once

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
    void reset();
    const uint8_t* renderViseme();

    // Noise threshold (adaptive)
    float getNoiseThreshold();

    // Current state
    VisemeType getCurrentViseme() const { return previousViseme; }
    float getEnvelope() const { return currentEnvelope; }
    float getGateThreshold() const { return adaptiveNoiseFloor * noiseGateMultiplier; }
    bool isLoudEnough() const { return currentEnvelope > getGateThreshold(); }

    // Get loudness level for display (0-60 range, based on envelope)
    uint16_t getLoudness() const {
        float loudness = mapFloat(currentEnvelope, adaptiveNoiseFloor, adaptiveNoiseFloor * 3.0f, 0, 60);
        return (uint16_t)constrain(loudness, 0, 60);
    }

    // Envelope parameters (BLE controllable)
    float getEnvelopeAttack() const { return envelopeAttack; }
    void setEnvelopeAttack(float value) { envelopeAttack = value; }
    float getEnvelopeRelease() const { return envelopeRelease; }
    void setEnvelopeRelease(float value) { envelopeRelease = value; }

    // Noise floor parameters (BLE controllable)
    float getNoiseFloorMin() const { return noiseFloorMin; }
    void setNoiseFloorMin(float value) { noiseFloorMin = value; }
    float getNoiseGateMultiplier() const { return noiseGateMultiplier; }
    void setNoiseGateMultiplier(float value) { noiseGateMultiplier = value; }

    // Viseme scale factors (BLE controllable)
    float getAhScale() const { return ahScale; }
    void setAhScale(float value) { ahScale = value; }
    float getEeScale() const { return eeScale; }
    void setEeScale(float value) { eeScale = value; }
    float getOhScale() const { return ohScale; }
    void setOhScale(float value) { ohScale = value; }
    float getOoScale() const { return ooScale; }
    void setOoScale(float value) { ooScale = value; }
    float getThScale() const { return thScale; }
    void setThScale(float value) { thScale = value; }

    float getLoudnessExponent() const { return loudnessExponent; }
    void setLoudnessExponent(float value) { loudnessExponent = value; }
    float getLoudnessSmoothing() const { return loudnessSmoothing; }
    void setLoudnessSmoothing(float value) { loudnessSmoothing = value; }
    float getLoudnessMax() const { return loudnessMax; }
    void setLoudnessMax(float value) { loudnessMax = value; }
    float getLoudnessMidBoost() const { return loudnessMidBoost; }
    void setLoudnessMidBoost(float value) { loudnessMidBoost = value; }

   private:
    I2SMicrophone mic;

    // ESP-DSP FFT buffers (using float for better performance)
    int16_t i2sBuffer[i2sSamples];     // Raw I2S samples
    float tempSamples[i2sSamples];     // Windowed samples (after DC removal + Hann window)
    float fftBuffer[2 * i2sSamples];   // Interleaved complex: [re0,im0,re1,im1,...]
    float hannWindow[i2sSamples];      // Pre-computed Hann window
    float magnitudes[i2sSamples / 2];  // Magnitude spectrum

    // Viseme amplitudes
    float ahAmplitude = 0, eeAmplitude = 0, ohAmplitude = 0;
    float ooAmplitude = 0, thAmplitude = 0;

    static const uint8_t visemeFramelength = 60;
    VisemeType previousViseme = AH;

    // Envelope Tracker
    float currentEnvelope = 0;
    float envelopeAttack = visemeEnvelopeAttack;
    float envelopeRelease = visemeEnvelopeRelease;

    // Adaptive Noise Floor
    float adaptiveNoiseFloor = visemeNoiseThreshold;
    float noiseFloorMin = visemeNoiseFloorMin;
    float noiseGateMultiplier = visemeNoiseGateMultiplier;

    // Attack Detection
    float previousEnvelope = 0;
    unsigned long lastAttackTime = 0;
    uint16_t minAttackInterval = visemeMinAttackInterval;

    // Viseme Scale Factors (normalization)
    float ahScale = visemeAhScale;
    float eeScale = visemeEeScale;
    float ohScale = visemeOhScale;
    float ooScale = visemeOoScale;
    float thScale = visemeThScale;

    // Loudness Level Parameters
    float loudnessExponent = visemeLoudnessExponent;
    float loudnessSmoothing = visemeLoudnessSmoothing;
    float loudnessMax = visemeLoudnessMax;
    float loudnessMidBoost = visemeLoudnessMidBoost;

    // Smoothed loudness level tracking
    float smoothedLoudness = 0.0f;

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

    // Initialization
    void initHannWindow();
    bool initFFT();

    // Audio processing pipeline
    void readAndPrepareSamples();
    void performFFT();
    void calculateMagnitudes();
    void analyzeVisemes();

    // Envelope tracking
    void updateEnvelope();
    float calculateRMS();

    // Adaptive noise floor
    void updateNoiseFloor();

    // Attack detection
    bool detectAttack();

    // Helper functions
    inline float binToFrequency(int bin);
    VisemeType getDominantViseme();
    const uint8_t* visemeOutput(VisemeType viseme, unsigned int level);
    void printDebugPlotter(unsigned int loudnessLevel = 0);

    // Loudness processing
    unsigned int calculateLoudnessLevel();
};
