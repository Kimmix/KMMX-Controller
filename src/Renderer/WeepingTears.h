#pragma once
#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "config.h"

class WeepingTears {
   public:
    WeepingTears(Hub75DMA* displayPtr = nullptr);

    void renderTears();
    void reset();
    void resetAll();
    void setIntensity(float intensity);  // 0.0 = light crying, 1.0 = heavy sobbing

   private:
    Hub75DMA* display;

    // Individual tear particle
    struct Tear {
        float xpos, ypos;          // Current position
        float velocityy;           // Falling speed
        uint8_t width;             // Width of this tear (in pixels)
        uint8_t height;            // Height/length of tear trail
        uint8_t brightness;        // Brightness variation (shimmer effect)
        uint8_t streamId;          // Which stream this tear belongs to
        bool active;               // Is this tear active?
    };

    // Tear stream (continuous flow)
    struct TearStream {
        uint8_t xpos;              // X position of stream
        bool active;               // Is stream flowing?
        unsigned long lastSpawn;   // Last spawn time
        uint16_t spawnInterval;    // Interval between drops in this stream
    };

    // Configuration
    static const uint8_t numStreamsPerEye = 1;  // 1 big stream per eye (2 total)
    static const uint8_t totalStreams = numStreamsPerEye * 2;
    static const uint8_t tearsPerStream = 20;   // More tears per stream for continuous big flow
    static const uint8_t totalTears = totalStreams * tearsPerStream;

    Tear tears[totalTears];
    TearStream streams[totalStreams];

    // Timing
    unsigned long lastUpdate = 0;
    float intensityLevel = 0.8;  // Default heavy crying for dramatic effect

    // Stream timing
    static const unsigned long baseSpawnInterval = 120;  // Base interval between drops in stream (ms) - continuous big stream

    // Helper methods
    void initializeStreams();
    void spawnTearInStream(uint8_t streamId);
    void updateTear(uint8_t index);
    void drawTear(const Tear& tear) const;

    void getRandomTearDimensions(uint8_t& width, uint8_t& height) const;
    uint16_t calculateSpawnInterval() const;
    uint8_t calculateBrightness() const;
};
