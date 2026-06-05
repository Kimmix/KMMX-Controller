#include "CheekPanel.h"
#include "config.h"

CheekPanel::CheekPanel(uint16_t numLeds, uint8_t pin)
    : strip(numLeds, pin, NEO_GRB + NEO_KHZ800),
      previousMillis(0),
      animStep(0),
      animDirection(1),
      centerPos(0),
      brightness(sideLEDBrightness) {}

void CheekPanel::configure(uint32_t color1, uint32_t color2, uint16_t updateInterval) {
    this->color1 = color1;
    this->color2 = color2;
    this->updateInterval = updateInterval;
}

void CheekPanel::init() {
    strip.begin();
    strip.setBrightness(brightness);

    // Initialize with color1
    uint32_t c1 = applyGamma(color1);
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c1);
    }
    strip.show();

    previousMillis = millis();
    // Start at random position (prefer split points for symmetry)
    centerPos = (esp_random() % 2) * (strip.numPixels() / 2);
}

void CheekPanel::setBrightness(uint8_t value) {
    brightness = value;
    strip.setBrightness(brightness);
}

uint8_t CheekPanel::getBrightness() const {
    return brightness;
}

void CheekPanel::setBackgroundColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    color1 = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void CheekPanel::setFadeColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    color2 = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t CheekPanel::getBackgroundColor() const {
    return color1;
}

uint32_t CheekPanel::getFadeColor() const {
    return color2;
}

void CheekPanel::update() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis < updateInterval) {
        return;
    }

    previousMillis = currentMillis;

    // Update animation step
    animStep += animDirection;

    // Check boundaries and reverse direction (0-39 range for ~8s cycle)
    if (animStep >= 39) {
        animDirection = -1;
        animStep = 39;
    } else if (animStep <= 0) {
        animDirection = 1;
        animStep = 0;
        // Pick new random center position (left or right side center)
        uint16_t halfStrip = strip.numPixels() / 2;
        centerPos = (esp_random() % 2) * halfStrip;
    }

    // Apply smooth easing to animation step
    uint8_t easedStep = easeInOutQuad(animStep);

    // Calculate expansion size based on eased step
    uint16_t numPixels = strip.numPixels();
    uint16_t halfStrip = numPixels / 2;
    uint8_t maxExpand = (halfStrip * 7) / 10;  // 70% of one side
    uint8_t expandSize = (easedStep * maxExpand) / 39;  // Scale to 0-maxExpand

    // Fill entire strip with color1 first
    uint32_t c1 = applyGamma(color1);
    for (uint16_t i = 0; i < numPixels; i++) {
        strip.setPixelColor(i, c1);
    }

    // Draw expanding color2 wave from center position
    uint32_t c2 = applyGamma(color2);

    // Expand symmetrically in both directions from center
    for (uint8_t offset = 0; offset < expandSize; offset++) {
        // Calculate positions wrapping around the strip
        uint16_t pos1 = (centerPos + offset) % numPixels;
        uint16_t pos2 = (centerPos - offset + numPixels) % numPixels;

        strip.setPixelColor(pos1, c2);
        if (pos1 != pos2) {  // Avoid setting center twice
            strip.setPixelColor(pos2, c2);
        }
    }

    strip.show();
}

uint32_t CheekPanel::applyGamma(uint32_t color) {
    uint8_t r = pgm_read_byte(&gammaTable[(color >> 16) & 0xFF]);
    uint8_t g = pgm_read_byte(&gammaTable[(color >> 8) & 0xFF]);
    uint8_t b = pgm_read_byte(&gammaTable[color & 0xFF]);
    return strip.Color(r, g, b);
}

uint8_t CheekPanel::easeInOutQuad(uint8_t t) {
    // Ease-in-out quadratic: slow start, fast middle, slow end
    // Input: 0-39, Output: 0-39
    // Uses integer math for efficiency
    if (t < 20) {
        // Ease in: t^2 / 20
        uint16_t temp = (uint16_t)t * t;
        return temp / 20;
    } else {
        // Ease out: 1 - (1-t)^2 / 20
        uint8_t inv = 39 - t;
        uint16_t temp = (uint16_t)inv * inv;
        return 39 - (temp / 20);
    }
}
