#include "RGBStatus.h"

#if ENABLE_RGB_STATUS_LED
// Full implementation when RGB Status LED is enabled
#ifdef KMMX_CONTROLLER_V4
// V4 uses SK6812 3535 RGB with RGB color order
RGBStatus::RGBStatus(int pin, int pixelCount)
    : pixel(pixelCount, pin, NEO_RGB + NEO_KHZ800), lastChangeTime(0), isOn(false) {}
#else
// V2 uses WS2812 with GRB color order
RGBStatus::RGBStatus(int pin, int pixelCount)
    : pixel(pixelCount, pin, NEO_GRB + NEO_KHZ800), lastChangeTime(0), isOn(false) {}
#endif

void RGBStatus::init() {
    pixel.begin();
    pixel.setBrightness(50);  // Reset to normal brightness
}

void RGBStatus::update() {
    if (isOn && (millis() - lastChangeTime >= 1500)) {
        turnOff();
    }
}

void RGBStatus::setColor(Color color) {
    switch (color) {
        case RED:
            setColor(255, 50, 50);
            break;
        case PINK:
            setColor(255, 192, 203);
            break;
        case LIGHT_PINK:
            setColor(150, 116, 122);
            break;
        case BLUE:
            setColor(173, 216, 230);
            break;
        case WHITE:
            setColor(255, 255, 255);
            break;
        case YELLOW:
            setColor(255, 255, 224);
            break;
        case CYAN:
            setColor(224, 255, 255);
            break;
        case MAGENTA:
            setColor(255, 182, 255);
            break;
    }
}

void RGBStatus::setColor(int r, int g, int b) {
    setColorInternal(pixel.Color(r, g, b));
}

void RGBStatus::turnOff() {
    setColorInternal(pixel.Color(0, 0, 0));
    isOn = false;
}

void RGBStatus::setColorInternal(uint32_t color) {
    uint8_t r = pgm_read_byte(&gammaTable[(color >> 16) & 0xFF]);
    uint8_t g = pgm_read_byte(&gammaTable[(color >> 8) & 0xFF]);
    uint8_t b = pgm_read_byte(&gammaTable[color & 0xFF]);

    // Set the corrected color to the pixel
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();

    lastChangeTime = millis();
    isOn = true;
}

#else
// No-op implementation when RGB Status LED is disabled (production builds)
RGBStatus::RGBStatus(int pin, int pixelCount) : dummyPin(pin) {
    // No initialization needed
}

void RGBStatus::init() {
    // No-op
}

void RGBStatus::update() {
    // No-op
}

void RGBStatus::setColor(Color color) {
    // No-op
}

void RGBStatus::setColor(int r, int g, int b) {
    // No-op
}

void RGBStatus::turnOff() {
    // No-op
}
#endif
