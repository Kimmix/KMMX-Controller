#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * @brief Color effects for LED matrix display
 *
 * This class provides various color effects and color modes for the LED matrix.
 * Each mode can generate different visual effects based on position and time.
 */
class ColorEffects {
   public:
    /**
     * @brief Constructor
     * @param width Panel width in pixels
     * @param height Panel height in pixels
     */
    ColorEffects(uint8_t width, uint8_t height);

    /**
     * @brief Update cached values for the current frame
     * Call this once at the beginning of each frame for better performance
     */
    void updateFrame();

    /**
     * @brief Get color based on the current color mode
     * @param mode Color mode:
     *        0 = Gradient (customizable via setGradientColors)
     *        1 = Spiral/Vortex (VRChat hypno style)
     *        2 = Plasma Effect (trippy interference patterns)
     *        3 = Radial Pulse (breathing effect from center)
     *        4 = Dual Spiral (clean rotating spiral effect)
     *        5 = Dual Circle (rotating concentric circles effect)
     * @param lightness Lightness/brightness value (0-255)
     * @param row Row index (Y coordinate)
     * @param col Column index (X coordinate)
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void getColor(uint8_t mode, uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Set gradient colors for Mode 0
     * Use this to customize the gradient or reset to defaults
     * Default: Top RGB(255,163,147) Bottom RGB(255,43,91)
     * @param topR Top color red component (0-255)
     * @param topG Top color green component (0-255)
     * @param topB Top color blue component (0-255)
     * @param bottomR Bottom color red component (0-255)
     * @param bottomG Bottom color green component (0-255)
     * @param bottomB Bottom color blue component (0-255)
     */
    void setGradientColors(uint8_t topR, uint8_t topG, uint8_t topB, uint8_t bottomR, uint8_t bottomG, uint8_t bottomB);

    /**
     * @brief Get the top gradient color
     */
    void getGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Get the bottom gradient color
     */
    void getGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /**
     * @brief Set dual spiral colors for Mode 4
     * Uses the same color setting as gradient (top color = spiral color, bottom unused)
     * @param spiralR Spiral color red component (0-255)
     * @param spiralG Spiral color green component (0-255)
     * @param spiralB Spiral color blue component (0-255)
     */
    void setDualSpiralColor(uint8_t spiralR, uint8_t spiralG, uint8_t spiralB);

    /**
     * @brief Set effect thickness for Mode 4 & 5 (affects band width/spacing)
     * @param thickness Thickness value (0-255), higher = thicker bands
     */
    void setEffectThickness(uint8_t thickness);

    /**
     * @brief Get the effect thickness
     */
    uint8_t getEffectThickness() const;

    /**
     * @brief Set effect rotation speed for Mode 4 & 5
     * @param speed Speed value (0-255), higher = faster rotation
     */
    void setEffectSpeed(uint8_t speed);

    /**
     * @brief Get the effect rotation speed
     */
    uint8_t getEffectSpeed() const;

    /**
     * @brief Set effect direction for Mode 4 & 5 (invert rotation)
     * @param inverted Direction inverted (0 = normal, 1 = inverted)
     */
    void setEffectDirectionInverted(uint8_t inverted);

    /**
     * @brief Get the effect direction inversion state
     * @return Direction inverted (0 = normal, 1 = inverted)
     */
    uint8_t getEffectDirectionInverted() const;

    /**
     * @brief Set dual circle colors for Mode 5
     * @param circleR Circle color red component (0-255)
     * @param circleG Circle color green component (0-255)
     * @param circleB Circle color blue component (0-255)
     */
    void setDualCircleColor(uint8_t circleR, uint8_t circleG, uint8_t circleB);

   private:
    uint8_t panelWidth;
    uint8_t panelHeight;

    // Global center points for effects (calculated from eye configuration in config.h)
    static constexpr float effectCenterLeftX = eyeOffsetX + eyeWidth / 2.0f;           // Left eye center
    static constexpr float effectCenterRightX = screenWidth - eyeOffsetX - eyeWidth / 2.0f; // Right eye center (mirrored)
    static constexpr float effectCenterY = eyeOffsetY + eyeHeight / 2.0f;              // Vertical center of eyes

    // Pre-calculated constants for performance optimization
    static constexpr float maxDistLeft = sqrtf(effectCenterLeftX * effectCenterLeftX + effectCenterY * effectCenterY);
    static constexpr float maxDistRight = sqrtf(effectCenterRightX * effectCenterRightX + effectCenterY * effectCenterY);

    // Cached time value (updated once per frame for performance)
    float cachedTime;

    // Gradient colors for Mode 0 (default KMMX colors, customizable)
    uint8_t gradientTopR = 255;
    uint8_t gradientTopG = 163;
    uint8_t gradientTopB = 147;
    uint8_t gradientBottomR = 255;
    uint8_t gradientBottomG = 43;
    uint8_t gradientBottomB = 91;

    // Dual spiral colors for Mode 4 (default cyan, customizable)
    uint8_t dualSpiralR = 0;
    uint8_t dualSpiralG = 255;
    uint8_t dualSpiralB = 255;
    uint8_t dualSpiralThickness = 128;  // Default thickness (0-255) - shared with dual circle
    uint8_t dualSpiralSpeed = 128;      // Default rotation speed (0-255) - shared with dual circle
    bool effectDirectionInverted = false; // Default direction (false = normal, true = inverted) - shared

    // Dual circle colors for Mode 5 (default magenta, customizable)
    uint8_t dualCircleR = 255;
    uint8_t dualCircleG = 0;
    uint8_t dualCircleB = 255;
    // Note: thickness, speed, and direction are shared with dual spiral (above)

    /**
     * @brief Mode 0: Customizable gradient
     */
    void modeGradient(uint8_t lightness, int row, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Mode 1: Spiral/Vortex effect with rainbow HSV cycling
     */
    void modeSpiralVortex(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Mode 2: Plasma effect with multiple sine wave interference
     */
    void modePlasmaEffect(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Mode 3: Radial pulse/breathing effect from center
     */
    void modeRadialPulse(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Mode 4: Dual spiral effect (customizable color and thickness)
     */
    void modeDualSpiral(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Mode 5: Dual circle effect (customizable color and thickness)
     */
    void modeDualCircle(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Helper: Convert HSV to RGB
     * @param h Hue (0.0 - 6.0)
     * @param s Saturation (0.0 - 1.0)
     * @param v Value/brightness (0.0 - 1.0)
     * @param r Reference to red component (output)
     * @param g Reference to green component (output)
     * @param b Reference to blue component (output)
     */
    void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Helper: Check if a pixel is in the eye region
     * @param row Row index (Y coordinate)
     * @param col Column index (X coordinate)
     * @return true if the pixel is within either eye region
     */
    bool isInEyeRegion(int row, int col) const;
};

