#include "WeepingTears.h"
#include <Arduino.h>

WeepingTears::WeepingTears(Hub75DMA* displayPtr) : display(displayPtr) {
    initializeStreams();
    resetAll();
}

void WeepingTears::renderTears() {
    unsigned long currentTime = millis();

    // Spawn new tears in each active stream
    for (uint8_t streamId = 0; streamId < totalStreams; streamId++) {
        if (streams[streamId].active &&
            currentTime - streams[streamId].lastSpawn >= streams[streamId].spawnInterval) {
            spawnTearInStream(streamId);
            streams[streamId].lastSpawn = currentTime;
        }
    }

    // Update tear positions every 25ms for smooth falling with motion blur
    if (currentTime - lastUpdate >= 25) {
        for (uint8_t i = 0; i < totalTears; i++) {
            updateTear(i);
        }
        lastUpdate = currentTime;
    }

    // Draw all active tears
    for (uint8_t i = 0; i < totalTears; i++) {
        if (tears[i].active) {
            drawTear(tears[i]);
        }
    }
}

void WeepingTears::initializeStreams() {
    unsigned long currentTime = millis();

    // Position streams at inner corner of each eye
    streams[0].xpos = eyeOffsetX + 3;
    streams[1].xpos = screenWidth - eyeOffsetX - 13;

    // Activate streams with randomized spawn intervals
    for (uint8_t i = 0; i < totalStreams; i++) {
        streams[i].active = true;
        streams[i].lastSpawn = currentTime;
        streams[i].spawnInterval = calculateSpawnInterval();
    }
}

void WeepingTears::spawnTearInStream(uint8_t streamId) {
    // Find an inactive tear slot within this stream's range
    uint8_t startIdx = streamId * tearsPerStream;
    uint8_t endIdx = startIdx + tearsPerStream;

    for (uint8_t i = startIdx; i < endIdx; i++) {
        if (!tears[i].active) {
            tears[i].xpos = streams[streamId].xpos;
            tears[i].ypos = eyeOffsetY + eyeHeight - 4;
            getRandomTearDimensions(tears[i].width, tears[i].height);
            tears[i].brightness = calculateBrightness();
            tears[i].velocityy = 0.4f + (intensityLevel * 0.3f);
            tears[i].streamId = streamId;
            tears[i].active = true;
            break;
        }
    }
}

void WeepingTears::updateTear(uint8_t index) {
    if (!tears[index].active) {
        return;
    }

    // Update falling position
    tears[index].ypos += tears[index].velocityy;

    // Random shimmer effect (12.5% chance each frame)
    if ((esp_random() & 0x07) == 0) {
        tears[index].brightness = calculateBrightness();
    }

    // Deactivate when tear falls off screen
    if (tears[index].ypos >= screenHeight + tears[index].height) {
        tears[index].active = false;
    }
}

void WeepingTears::drawTear(const Tear& tear) const {
    // Light cyan/blue tear color - scales with brightness
    uint8_t r = (tear.brightness * 100) / 255;    // Max red: ~39% of brightness
    uint8_t g = (tear.brightness * 200) / 255;    // Max green: ~78% of brightness
    uint8_t b = tear.brightness;                   // Full brightness

    int x = (int)tear.xpos;
    int y = (int)tear.ypos;

    // Draw tear trail with gradient from faint (top) to bright (bottom)
    for (uint8_t row = 0; row < tear.height; row++) {
        // Calculate brightness gradient: starts at ~10% at top, reaches 100% at bottom
        uint8_t gradientFactor = (row * 255) / (tear.height - 1);
        uint8_t pixelR = (r * gradientFactor) / 255;
        uint8_t pixelG = (g * gradientFactor) / 255;
        uint8_t pixelB = (b * gradientFactor) / 255;

        // Draw horizontal line for this row of the tear
        for (uint8_t col = 0; col < tear.width; col++) {
            int px = x + col;
            int py = y + row;

            // Bounds check
            if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
                display->drawPixel(px, py, display->color565(pixelR, pixelG, pixelB));
            }
        }
    }
}

void WeepingTears::reset() {
    unsigned long currentTime = millis();

    // Restart all streams with updated intervals
    for (uint8_t i = 0; i < totalStreams; i++) {
        streams[i].active = true;
        streams[i].lastSpawn = currentTime;
        streams[i].spawnInterval = calculateSpawnInterval();
    }
}

void WeepingTears::resetAll() {
    // Deactivate all tears
    for (uint8_t i = 0; i < totalTears; i++) {
        tears[i].active = false;
    }

    // Deactivate all streams
    for (uint8_t i = 0; i < totalStreams; i++) {
        streams[i].active = false;
        streams[i].lastSpawn = 0;
    }

    lastUpdate = millis();
}

void WeepingTears::setIntensity(float intensity) {
    intensityLevel = constrain(intensity, 0.0f, 1.0f);

    // Update spawn intervals for all streams
    for (uint8_t i = 0; i < totalStreams; i++) {
        streams[i].spawnInterval = calculateSpawnInterval();
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void WeepingTears::getRandomTearDimensions(uint8_t& width, uint8_t& height) const {
    uint32_t randomNum = esp_random();
    uint8_t value = randomNum % 100;

    if (value < 50) {
        // 50% small tears - narrow and short
        width = 3;
        height = 6;
    } else if (value < 85) {
        // 35% medium tears
        width = 6;
        height = 8;
    } else {
        // 15% large tears - wide and long for dramatic effect
        width = 10;
        height = 10;
    }
}

uint16_t WeepingTears::calculateSpawnInterval() const {
    // Higher intensity = faster spawning (shorter interval)
    uint16_t adjustedInterval = baseSpawnInterval / (0.5f + intensityLevel);
    // Add randomness for natural variation (-10 to +9)
    return adjustedInterval + (esp_random() % 20) - 10;
}

uint8_t WeepingTears::calculateBrightness() const {
    // Bright shimmer range: 220-255
    return 220 + (esp_random() % 36);
}
