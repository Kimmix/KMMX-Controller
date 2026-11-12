#include "SSD1306.h"

SSD1306::SSD1306() {
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
}

SSD1306::~SSD1306() {
    if (u8g2) {
        delete u8g2;
        u8g2 = nullptr;
    }
}

bool SSD1306::setup() {
    if (!u8g2) {
        return false;
    }

    // Initialize display
    u8g2->begin();

    // Clear the display
    u8g2->clearBuffer();
    u8g2->sendBuffer();

    // Set default font
    u8g2->setFont(u8g2_font_ncenB08_tr);

    return true;
}

void SSD1306::clear() {
    if (!u8g2) return;
    u8g2->clearBuffer();
}

void SSD1306::update() {
    if (!u8g2) return;
    u8g2->sendBuffer();
}

void SSD1306::drawText(int16_t x, int16_t y, const char* text) {
    if (!u8g2) return;
    u8g2->drawStr(x, y, text);
}

void SSD1306::setFont(const uint8_t* font) {
    if (!u8g2) return;
    u8g2->setFont(font);
}

void SSD1306::drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!u8g2) return;
    u8g2->drawBox(x, y, w, h);
}

void SSD1306::drawFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!u8g2) return;
    u8g2->drawFrame(x, y, w, h);
}

void SSD1306::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h) {
    if (!bitmap) return;

    // Draw grayscale bitmap pixel by pixel
    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            uint8_t pixel = pgm_read_byte(bitmap + row * w + col);
            // Only draw if pixel is bright enough (binary threshold for monochrome display)
            if (pixel > 127) {  // Threshold at 50% brightness
                u8g2->drawPixel(x + col, y + row);
            }
        }
    }
}
