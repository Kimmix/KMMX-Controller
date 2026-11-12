#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"

class SSD1306 {
   private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2;

   public:
    /**
     * @brief Constructor for the SSD1306 class.
     */
    SSD1306();

    /**
     * @brief Destructor to clean up allocated memory.
     */
    ~SSD1306();

    /**
     * @brief Initialize the SSD1306 display.
     * @return true if initialization was successful, false otherwise
     */
    bool setup();

    /**
     * @brief Clear the display buffer.
     */
    void clear();

    /**
     * @brief Send buffer to display (must call after drawing).
     */
    void update();

    /**
     * @brief Draw text at position.
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param text Text to display
     */
    void drawText(int16_t x, int16_t y, const char* text);

    /**
     * @brief Set font for text drawing.
     * @param font U8g2 font (e.g., u8g2_font_ncenB08_tr)
     */
    void setFont(const uint8_t* font);

    /**
     * @brief Draw a box (filled rectangle).
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     */
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h);

    /**
     * @brief Draw a frame (rectangle outline).
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     */
    void drawFrame(int16_t x, int16_t y, int16_t w, int16_t h);

    /**
     * @brief Draw a grayscale bitmap from PROGMEM.
     * @param x X coordinate
     * @param y Y coordinate
     * @param bitmap Pointer to bitmap in PROGMEM
     * @param w Width of bitmap
     * @param h Height of bitmap
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h);

    /**
     * @brief Get pointer to U8G2 object for advanced operations.
     * @return Pointer to u8g2 object or nullptr if not initialized
     */
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* getU8g2() { return u8g2; }
};
