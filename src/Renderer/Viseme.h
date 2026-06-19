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
    const uint8_t* renderViseme();

    // Noise threshold (adaptive)
    float getNoiseThreshold();
    void setNoiseThreshold(float value);

    // Current state
    VisemeType getCurrentViseme() const { return previousViseme; }
    float getEnvelope() const { return currentEnvelope; }
    float getGateThreshold() const { return adaptiveNoiseFloor; }
    bool isLoudEnough() const { return currentEnvelope > getGateThreshold(); }

    // Get loudness level for display (0-20 range, based on envelope)
    uint16_t getLoudness() const {
        float loudness = mapFloat(currentEnvelope, adaptiveNoiseFloor, adaptiveNoiseFloor * 3.0f, 0, 20);
        return (uint16_t)constrain(loudness, 0, 20);
    }

    // Envelope parameters (BLE controllable)
    float getEnvelopeAttack() const { return envelopeAttack; }
    void setEnvelopeAttack(float value) { envelopeAttack = value; }
    float getEnvelopeRelease() const { return envelopeRelease; }
    void setEnvelopeRelease(float value) { envelopeRelease = value; }

    // Noise floor parameters (BLE controllable)
    float getNoiseFloorMin() const { return noiseFloorMin; }
    void setNoiseFloorMin(float value) { noiseFloorMin = value; }
    float getNoiseFloorMax() const { return noiseFloorMax; }
    void setNoiseFloorMax(float value) { noiseFloorMax = value; }
    float getNoiseAdaptSpeed() const { return noiseAdaptSpeed; }
    void setNoiseAdaptSpeed(float value) { noiseAdaptSpeed = value; }

    // Attack detection parameters (BLE controllable)
    float getAttackThreshold() const { return attackThreshold; }
    void setAttackThreshold(float value) { attackThreshold = value; }
    // Confidence parameters (BLE controllable)
    float getMinSeparation() const { return minSeparation; }
    void setMinSeparation(float value) { minSeparation = value; }

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

    static const uint8_t visemeFramelength = 20;  // Frame count (to expand, change this and add more frames to arrays)
    VisemeType previousViseme;

    // Envelope Tracker
    float currentEnvelope = 0;
    float envelopeAttack = visemeEnvelopeAttack;
    float envelopeRelease = visemeEnvelopeRelease;

    // Adaptive Noise Floor
    float adaptiveNoiseFloor = visemeNoiseThreshold;
    float noiseFloorMin = visemeNoiseFloorMin;
    float noiseFloorMax = visemeNoiseFloorMax;
    float noiseAdaptSpeed = visemeNoiseAdaptSpeed;

    // Attack Detection
    float previousEnvelope = 0;
    float attackThreshold = visemeAttackThreshold;
    unsigned long lastAttackTime = 0;
    uint16_t minAttackInterval = visemeMinAttackInterval;
    bool attackDetected = false;

    // Confidence
    float minSeparation = visemeMinSeparation;

    // Viseme Scale Factors (normalization)
    float ahScale = visemeAhScale;
    float eeScale = visemeEeScale;
    float ohScale = visemeOhScale;
    float ooScale = visemeOoScale;
    float thScale = visemeThScale;

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
    void printDebugPlotter(float distinctiveness = 0, unsigned int loudnessLevel = 0, bool canChange = false);
};
