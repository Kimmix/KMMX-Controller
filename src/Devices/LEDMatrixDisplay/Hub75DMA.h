#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"
#include "Bitmaps/Bitmaps.h"
#include "ColorEffects.h"

class Hub75DMA {
   private:
    bool ready = false;
    MatrixPanel_I2S_DMA* matrix;  // Pointer to the matrix panel object
    const uint8_t panelWidth = panelResX;   // Panel width from config
    const uint8_t panelHeight = panelResY;  // Panel height from config
    uint8_t panelBrightness = panelInitBrightness; // Default brightness

    // Color effects engine
    ColorEffects colorEffects = ColorEffects(panelResX, panelResY);
    uint8_t colorMode = 0;  // 0 = default gradient, 1 = spiral/vortex, etc.

    // Last rendered bitmaps for OLED mirroring
    const uint8_t* lastEyeBitmap = nullptr;
    const uint8_t* lastMouthBitmap = nullptr;

    // Glitch effect state tracking
    struct GlitchState {
        bool active = false;              // Whether glitch is currently active
        bool isFullScreen = false;        // Cached full-screen flag
        unsigned long startTime = 0;      // When glitch was triggered (ms)
        unsigned long lastUpdate = 0;     // Last time glitch params were randomized (ms)
        unsigned long duration = 250;     // How long glitch lasts (ms)
        int baseIntensity = 50;           // Original glitch strength (0-100)
        float currentIntensity = 1.0f;    // Current intensity with fade-out and ramp-up (0.0-1.0)
        uint8_t activeRowCount = 0;       // Number of active glitch rows (current frame)
        uint8_t maxRowLimit = 8;          // Maximum allowed rows for this glitch instance (1-8, intensity-based)

        // Multi-row glitch support (up to 8 independent glitch rows)
        static const int MAX_GLITCH_ROWS = 8;
        int glitchRow[MAX_GLITCH_ROWS] = {-1, -1, -1, -1, -1, -1, -1, -1};
        int glitchShift[MAX_GLITCH_ROWS] = {0, 0, 0, 0, 0, 0, 0, 0};
        int verticalJitter[MAX_GLITCH_ROWS] = {0, 0, 0, 0, 0, 0, 0, 0};

        int glitchRadius = 2;             // How many rows around each glitch row are affected
    } glitchState;

    // Intensity calculation cache (updated by updateGlitchParams)
    struct IntensityCache {
        float normalized = 0.5f;          // normalizedIntensity
        float squared = 0.25f;            // intensity^2
        float cubed = 0.125f;             // intensity^3
        float quad = 0.0625f;             // intensity^4
    } intensityCache;

    // Helper: Calculate and cache intensity power curves
    inline void calculateIntensityCurves(float baseIntensity, IntensityCache& cache) {
        cache.normalized = baseIntensity / 100.0f;
        cache.squared = cache.normalized * cache.normalized;
        cache.cubed = cache.squared * cache.normalized;
        cache.quad = cache.squared * cache.squared;
    }

    // Helper: Update glitch parameters (shared logic between trigger and update)
    void updateGlitchParams(const IntensityCache& cache, bool isInitialTrigger);

    // Combined getter: Get both horizontal offset and vertical jitter for a row
    // Returns via reference parameters for efficiency (avoids struct return overhead)
    inline void getGlitchEffects(int row, int& outHorizontalShift, int& outVerticalJitter) const {
        outHorizontalShift = 0;
        outVerticalJitter = 0;

        if (!glitchState.active) return;

        // Early exit optimization: iterate only over active rows
        for (uint8_t i = 0; i < glitchState.activeRowCount; i++) {
            // Full-screen mode or proximity check
            if (glitchState.isFullScreen || abs(row - glitchState.glitchRow[i]) < glitchState.glitchRadius) {
                outHorizontalShift = glitchState.glitchShift[i];
                outVerticalJitter = glitchState.verticalJitter[i];
                return;
            }
        }
    }

    // Helper functions to generate colors and patterns
    /**
     * @brief Generates a color based on the lightness, row, and column.
     * @param lightness Lightness value (0-255)
     * @param row Row index
     * @param col Column index
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getColorMap(const uint8_t lightness, const int row, const int col, uint8_t& r, uint8_t& g, uint8_t& b);

   public:
    /**
     * @brief Constructor for the Hub75DMA class.
     */
    Hub75DMA();
    bool isReady() const { return ready; }

    int getRefreshRate();

    /**
     * @brief Renders the buffer to the screen.
     */
    void flipDMABuffer();

    /**
     * @brief Clears the screen.
     */
    void clearScreen();

    /**
     * @brief Updates color effects cache for the current frame.
     * Call this once at the beginning of each frame for better performance.
     */
    void updateColorEffectsFrame();

    /**
     * @brief Gets the current brightness value.
     * @return Current brightness value.
     */
    int getBrightnessValue();

    /**
     * @brief Converts RGB values to 565 format.
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @return Color in 565 format.
     */
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Draws a pixel.
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Color of the pixel in 565 format
     */
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    /**
     * @brief Sets the brightness value.
     * @param value Brightness value (0-255)
     */
    void setBrightnessValue(uint8_t value);

    /**
     * @brief Sets the color mode.
     * @param mode Color mode:
     *        0 = Gradient (customizable via setGradientColors)
     *        1 = Spiral/Vortex (VRChat hypno style)
     *        2 = Plasma Effect (trippy interference patterns)
     *        3 = Radial Pulse (breathing effect from center)
     *        4 = Dual Spiral (customizable color and thickness)
     *        5 = Dual Circle (rotating concentric circles effect)
     */
    void setColorMode(uint8_t mode);

    /**
     * @brief Gets the current color mode.
     * @return Current color mode
     */
    uint8_t getColorMode() const;

    /**
     * @brief Sets the custom gradient colors.
     * @param topR Top color red component (0-255)
     * @param topG Top color green component (0-255)
     * @param topB Top color blue component (0-255)
     * @param bottomR Bottom color red component (0-255)
     * @param bottomG Bottom color green component (0-255)
     * @param bottomB Bottom color blue component (0-255)
     */
    void setGradientColors(uint8_t topR, uint8_t topG, uint8_t topB, uint8_t bottomR, uint8_t bottomG, uint8_t bottomB);

    /**
     * @brief Gets the top gradient color.
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Gets the bottom gradient color.
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Sets the dual spiral color for Mode 4.
     * @param spiralR Spiral color red component (0-255)
     * @param spiralG Spiral color green component (0-255)
     * @param spiralB Spiral color blue component (0-255)
     */
    void setDualSpiralColor(uint8_t spiralR, uint8_t spiralG, uint8_t spiralB);

    /**
     * @brief Gets the dual spiral color.
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getDualSpiralColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Sets the dual circle color for Mode 5.
     * @param circleR Circle color red component (0-255)
     * @param circleG Circle color green component (0-255)
     * @param circleB Circle color blue component (0-255)
     */
    void setDualCircleColor(uint8_t circleR, uint8_t circleG, uint8_t circleB);

    /**
     * @brief Gets the dual circle color.
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getDualCircleColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Sets the effect thickness for Mode 4 & 5 (band width/spacing).
     * @param thickness Thickness value (0-255)
     */
    void setEffectThickness(uint8_t thickness);

    /**
     * @brief Gets the effect thickness.
     * @return Thickness value (0-255)
     */
    uint8_t getEffectThickness() const;

    /**
     * @brief Sets the effect rotation speed for Mode 4 & 5.
     * @param speed Speed value (0-255)
     */
    void setEffectSpeed(uint8_t speed);

    /**
     * @brief Gets the effect rotation speed.
     * @return Speed value (0-255)
     */
    uint8_t getEffectSpeed() const;

    /**
     * @brief Sets the effect direction inversion for Mode 4 & 5.
     * @param inverted Direction inverted (0 = normal, 1 = inverted)
     */
    void setEffectDirectionInverted(uint8_t inverted);

    /**
     * @brief Gets the effect direction inversion state.
     * @return Direction inverted (0 = normal, 1 = inverted)
     */
    uint8_t getEffectDirectionInverted() const;

    // Glitch effect control
    /**
     * @brief Triggers a screen glitch effect with exponential intensity scaling.
     *
     * Creates a multi-layered visual distortion with multiple simultaneous glitch rows,
     * horizontal pixel shifts, vertical jitter, and smooth fade-out ending.
     *
     * Scaling behavior:
     * - Active glitch rows: cubic scaling (intensity^3)
     * - Glitch radius: quadratic scaling (intensity^2)
     * - Full-screen chance: quadratic scaling (intensity^2)
     * - Duration: quadratic scaling (intensity^2)
     * - Horizontal pixel shift: quartic scaling (intensity^4)
     * - Vertical jitter: quadratic scaling (intensity^2)
     *
     * @param intensity Glitch strength (0-100, default: 50)
     */
    void triggerGlitch(int intensity = 50);

    /**
     * @brief Updates the glitch effect state each frame.
     *
     * Randomizes glitch parameters periodically for dynamic effect.
     * Handles fade-out during final portion of duration.
     * Must be called every frame for smooth animation.
     */
    void updateGlitch();

    /**
     * @brief Checks if glitch effect is currently active.
     * @return true if glitch is active, false otherwise
     */
    bool isGlitchActive() const;

    // Drawing functions for various bitmaps
    /**
     * @brief Draws a bitmap fullscreen.
     * @param bitmap Bitmap array
     */
    void drawFullscreen(const uint8_t* bitmap);

    /**
     * @brief Draws an eye bitmap.
     * @param bitmap Bitmap array
     */
    void drawEye(const uint8_t* bitmap);

    /**
     * @brief Draws left and right eye bitmaps.
     * @param bitmapL Left eye bitmap array
     * @param bitmapR Right eye bitmap array
     */
    void drawEye(const uint8_t* bitmapL, const uint8_t* bitmapR);

    /**
     * @brief Draws an eye pupil bitmap at specified coordinates.
     * @param bitmap Bitmap array
     * @param x X coordinate
     * @param y Y coordinate
     */
    void drawEyePupil(const uint8_t* bitmap, int x, int y);

    /**
     * @brief Draws a nose bitmap.
     * @param bitmap Bitmap array
     */
    void drawNose(const uint8_t* bitmap);

    /**
     * @brief Draws a mouth bitmap.
     * @param bitmap Bitmap array
     */
    void drawMouth(const uint8_t* bitmap);

    /**
     * @brief Gets the last drawn eye bitmap pointer.
     * @return Pointer to the last eye bitmap, or nullptr if none drawn yet
     */
    const uint8_t* getLastEyeBitmap() const { return lastEyeBitmap; }

    /**
     * @brief Gets the last drawn mouth bitmap pointer.
     * @return Pointer to the last mouth bitmap, or nullptr if none drawn yet
     */
    const uint8_t* getLastMouthBitmap() const { return lastMouthBitmap; }

    /**
     * @brief Draws a bitmap at specified coordinates.
     * @param bitmap Bitmap array
     * @param imageWidth Width of the bitmap
     * @param imageHeight Height of the bitmap
     * @param offsetX X offset
     * @param offsetY Y offset
     */
    void drawBitmap(const uint8_t* bitmap, int imageWidth, int imageHeight, int offsetX, int offsetY);

    /**
     * @brief Draws left and right bitmaps at specified coordinates.
     * @param bitmapL Left bitmap array
     * @param bitmapR Right bitmap array
     * @param imageWidth Width of the bitmaps
     * @param imageHeight Height of the bitmaps
     * @param offsetX X offset
     * @param offsetY Y offset
     */
    void drawBitmap(const uint8_t* bitmapL, const uint8_t* bitmapR, int imageWidth, int imageHeight, int offsetX, int offsetY);

    /**
     * @brief Draws a bitmap with specified color at specified coordinates.
     * @param bitmap Bitmap array
     * @param imageWidth Width of the bitmap
     * @param imageHeight Height of the bitmap
     * @param offsetX X offset
     * @param offsetY Y offset
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     */
    void drawBitmap(const uint8_t* bitmap, int imageWidth, int imageHeight, int offsetX, int offsetY, uint8_t r, uint8_t g, uint8_t b);
};
