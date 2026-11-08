#include "ColorEffects.h"
#include "Bitmaps/Bitmaps.h"
#include <math.h>

ColorEffects::ColorEffects(uint8_t width, uint8_t height)
    : panelWidth(width), panelHeight(height), cachedTime(0.0f) {
}

void ColorEffects::updateFrame() {
    cachedTime = millis() * 0.001f;  // Cache time once per frame
}

void ColorEffects::getColor(uint8_t mode, uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (mode) {
        case 0:
            modeGradient(lightness, row, r, g, b);
            break;
        case 1:
            modeSpiralVortex(lightness, row, col, r, g, b);
            break;
        case 2:
            modePlasmaEffect(lightness, row, col, r, g, b);
            break;
        case 3:
            modeRadialPulse(lightness, row, col, r, g, b);
            break;
        case 4:
            modeDualSpiral(lightness, row, col, r, g, b);
            break;
        case 5:
            modeDualCircle(lightness, row, col, r, g, b);
            break;
        default:
            // Fallback to gradient
            modeGradient(lightness, row, r, g, b);
            break;
    }
}

void ColorEffects::setGradientColors(uint8_t topR, uint8_t topG, uint8_t topB, uint8_t bottomR, uint8_t bottomG, uint8_t bottomB) {
    gradientTopR = topR;
    gradientTopG = topG;
    gradientTopB = topB;
    gradientBottomR = bottomR;
    gradientBottomG = bottomG;
    gradientBottomB = bottomB;
}

void ColorEffects::getGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = gradientTopR;
    g = gradientTopG;
    b = gradientTopB;
}

void ColorEffects::getGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = gradientBottomR;
    g = gradientBottomG;
    b = gradientBottomB;
}

void ColorEffects::setDualSpiralColor(uint8_t spiralR, uint8_t spiralG, uint8_t spiralB) {
    dualSpiralR = spiralR;
    dualSpiralG = spiralG;
    dualSpiralB = spiralB;
}

void ColorEffects::getDualSpiralColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = dualSpiralR;
    g = dualSpiralG;
    b = dualSpiralB;
}

void ColorEffects::setEffectThickness(uint8_t thickness) {
    dualSpiralThickness = thickness;
}

uint8_t ColorEffects::getEffectThickness() const {
    return dualSpiralThickness;
}

void ColorEffects::setEffectSpeed(uint8_t speed) {
    dualSpiralSpeed = speed;
}

uint8_t ColorEffects::getEffectSpeed() const {
    return dualSpiralSpeed;
}

void ColorEffects::setEffectDirectionInverted(uint8_t inverted) {
    effectDirectionInverted = (inverted != 0);
}

uint8_t ColorEffects::getEffectDirectionInverted() const {
    return effectDirectionInverted ? 1 : 0;
}

void ColorEffects::setDualCircleColor(uint8_t circleR, uint8_t circleG, uint8_t circleB) {
    dualCircleR = circleR;
    dualCircleG = circleG;
    dualCircleB = circleB;
}

void ColorEffects::getDualCircleColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    r = dualCircleR;
    g = dualCircleG;
    b = dualCircleB;
}

// ============================================================================
// Mode 0: Customizable Gradient
// ============================================================================
void ColorEffects::modeGradient(uint8_t lightness, int row, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Interpolate between top and bottom colors based on row
    // row 0 = top color, row 31 = bottom color
    const int clampedRow = constrain(row, 0, 31);
    const uint16_t fraction = (clampedRow << 8) / 31;  // 0-256 range

    // Interpolate each color component
    const uint16_t invFraction = 256 - fraction;
    const uint8_t baseR = (gradientTopR * invFraction + gradientBottomR * fraction) >> 8;
    const uint8_t baseG = (gradientTopG * invFraction + gradientBottomG * fraction) >> 8;
    const uint8_t baseB = (gradientTopB * invFraction + gradientBottomB * fraction) >> 8;

    // Apply lightness/brightness
    const uint16_t factor = lightness << 8;
    r = (factor * baseR) >> 16;
    g = (factor * baseG) >> 16;
    b = (factor * baseB) >> 16;
}

// ============================================================================
// Mode 1: Spiral/Vortex Effect (VRChat Hypno Shader Inspired)
// ============================================================================
void ColorEffects::modeSpiralVortex(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Determine which eye center to use based on horizontal position (split at single panel width)
    const float centerX = (col < panelResX) ? effectCenterLeftX : effectCenterRightX;

    // Calculate position relative to the nearest eye center
    const float dx = col - centerX;
    const float dy = row - effectCenterY;

    // Calculate angle and distance from center
    const float angle = atan2f(dy, dx);
    const float distance = sqrtf(dx * dx + dy * dy);

    // Create animated spiral effect (using cached time for performance)
    const float spiralSpeed = 2.0f;        // Rotation speed
    const float spiralTightness = 0.15f;   // How tight the spiral is (lower = wider)

    // Combine angle and distance for spiral pattern
    float spiralValue = angle + distance * spiralTightness - cachedTime * spiralSpeed;

    // Normalize to 0-1 range using sine wave for smooth cycling
    float hue = fmodf(spiralValue / (2.0f * PI) + 1.0f, 1.0f);

    // Add radial pulse effect (using cached time for performance)
    float pulse = sinf(distance * 0.1f - cachedTime * 3.0f) * 0.5f + 0.5f;

    // Convert HSV to RGB (hue cycling rainbow effect)
    // Hue: 0-1, Saturation: 1.0, Value: based on pulse and lightness
    float h = hue * 6.0f;
    float saturation = 1.0f;
    float value = (pulse * 0.3f + 0.7f) * (lightness / 255.0f);

    hsvToRgb(h, saturation, value, r, g, b);
}

// ============================================================================
// Mode 2: Plasma Effect (Demoscene Classic)
// ============================================================================
void ColorEffects::modePlasmaEffect(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Determine which eye center to use based on horizontal position (split at single panel width)
    const float centerX = (col < panelResX) ? effectCenterLeftX : effectCenterRightX;

    // Normalize coordinates to center (using nearest eye center)
    const float x = (col - centerX) / centerX;
    const float y = (row - effectCenterY) / effectCenterY;

    // Multiple sine waves creating interference patterns (using cached time for performance)
    float plasma1 = sinf(x * 5.0f + cachedTime);
    float plasma2 = sinf(y * 5.0f + cachedTime * 1.3f);
    float plasma3 = sinf((x + y) * 4.0f + cachedTime * 0.8f);
    float plasma4 = sinf(sqrtf(x * x + y * y) * 6.0f - cachedTime * 2.0f);

    // Combine all plasma components
    float plasmaValue = (plasma1 + plasma2 + plasma3 + plasma4) / 4.0f;

    // Convert to 0-1 range
    plasmaValue = (plasmaValue + 1.0f) * 0.5f;

    // Map to rainbow hue
    float hue = plasmaValue * 6.0f;
    float saturation = 0.9f;  // Slightly less saturated for smoother look
    float value = (lightness / 255.0f) * 0.9f + 0.1f;  // Keep some minimum brightness

    hsvToRgb(hue, saturation, value, r, g, b);
}

// ============================================================================
// Mode 3: Radial Pulse (Breathing Effect)
// ============================================================================
void ColorEffects::modeRadialPulse(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Determine which eye center to use based on horizontal position (split at single panel width)
    const float centerX = (col < panelResX) ? effectCenterLeftX : effectCenterRightX;
    const float maxDist = (col < panelResX) ? maxDistLeft : maxDistRight;

    // Calculate distance from center (using nearest eye center)
    const float dx = col - centerX;
    const float dy = row - effectCenterY;
    const float distance = sqrtf(dx * dx + dy * dy);

    // Normalize distance using pre-calculated constant
    const float normalizedDist = distance / maxDist;  // 0 at center, 1 at corners

    // Create breathing pulse from center (using cached time for performance)
    const float breathSpeed = 1.5f;
    const float breathOffset = sinf(cachedTime * breathSpeed) * 0.5f + 0.5f;  // 0-1

    // Create expanding rings
    const float ringSpeed = 2.0f;
    const float ringFrequency = 5.0f;
    const float rings = sinf(normalizedDist * ringFrequency - cachedTime * ringSpeed) * 0.5f + 0.5f;

    // Combine breath and rings
    const float pulse = (breathOffset * 0.6f + rings * 0.4f);

    // Color cycling - slower than spiral
    const float hue = fmodf(cachedTime * 0.5f + normalizedDist * 2.0f, 1.0f) * 6.0f;
    const float saturation = 0.85f;
    const float value = (pulse * 0.3f + 0.7f) * (lightness / 255.0f);  // Min 70% brightness

    hsvToRgb(hue, saturation, value, r, g, b);
}

// ============================================================================
// Mode 4: Dual Spiral Effect (Customizable colors, thickness, and speed)
// ============================================================================
void ColorEffects::modeDualSpiral(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    const float intensity = (lightness / 255.0f);

    // Check if pixel is in eye region - if not, use solid color (gradient bottom color)
    if (!isInEyeRegion(row, col)) {
        r = (uint8_t)(gradientBottomR * intensity);
        g = (uint8_t)(gradientBottomG * intensity);
        b = (uint8_t)(gradientBottomB * intensity);
        return;
    }

    // Pixel is in eye region - apply the spiral effect
    // Determine which eye center to use based on horizontal position (split at single panel width)
    const float centerX = (col < panelResX) ? effectCenterLeftX : effectCenterRightX;

    // Calculate position relative to the nearest eye center
    const float dx = col - centerX;
    const float dy = row - effectCenterY;

    // Calculate angle and distance from center
    const float angle = atan2f(dy, dx);
    const float distance = sqrtf(dx * dx + dy * dy);

    // Create rotating spiral pattern (using cached time for performance)
    const float spiralArms = 12.0f;        // Number of spiral arms/bands

    // Speed control: map 0-255 to rotation speed (0.2 to 3.0)
    // 0 = very slow (0.2), 128 = medium (1.5), 255 = very fast (3.0)
    const float rotationSpeed = 0.2f + (dualSpiralSpeed / 255.0f) * 2.8f;

    // Direction control: invert rotation direction if enabled
    const float directionMultiplier = effectDirectionInverted ? -1.0f : 1.0f;

    // Thickness control: map 0-255 to distance multiplier (INVERTED for intuitive control)
    // Higher values = thicker/wider bands (fewer bands in same space)
    // Remapped range: effective range is like 4-15 instead of 1-10
    // 0 = thin bands (1.2), 128 = medium (0.6), 255 = very thick/wide bands (0.02)
    const float thicknessFactor = 1.2f - (dualSpiralThickness / 255.0f) * 1.18f;

    // Combine angle with time for rotation, add distance for spiral effect
    float spiralValue = angle * spiralArms + distance * thicknessFactor - cachedTime * rotationSpeed * directionMultiplier * 2.0f * PI;

    // Create bands using sine wave - gives the alternating pattern
    float bands = sinf(spiralValue);

    // Threshold to create sharp edges between colors
    const float threshold = 0.0f;

    if (bands > threshold) {
        // Use Color 1 (spiral primary color)
        r = (uint8_t)(dualSpiralR * intensity);
        g = (uint8_t)(dualSpiralG * intensity);
        b = (uint8_t)(dualSpiralB * intensity);
    } else {
        // Use Color 2 (gradient bottom / secondary color)
        r = (uint8_t)(gradientBottomR * intensity);
        g = (uint8_t)(gradientBottomG * intensity);
        b = (uint8_t)(gradientBottomB * intensity);
    }
}

// ============================================================================
// Mode 5: Dual Circle Effect (Customizable colors, thickness, and speed)
// ============================================================================
void ColorEffects::modeDualCircle(uint8_t lightness, int row, int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    const float intensity = (lightness / 255.0f);

    // Check if pixel is in eye region - if not, use solid color (gradient bottom color)
    if (!isInEyeRegion(row, col)) {
        r = (uint8_t)(gradientBottomR * intensity);
        g = (uint8_t)(gradientBottomG * intensity);
        b = (uint8_t)(gradientBottomB * intensity);
        return;
    }

    // Pixel is in eye region - apply the circle effect
    // Determine which eye center to use based on horizontal position (split at single panel width)
    const float centerX = (col < panelResX) ? effectCenterLeftX : effectCenterRightX;

    // Calculate position relative to the nearest eye center
    const float dx = col - centerX;
    const float dy = row - effectCenterY;

    // Calculate distance from center
    const float distance = sqrtf(dx * dx + dy * dy);             // Number of circle bands

    // Speed control: map 0-255 to rotation speed (2.0 to 20.0)
    // 0 = very slow (2.0), 128 = medium (10.0), 255 = very fast (20.0)
    const float rotationSpeed = 2.0f + (dualSpiralSpeed / 255.0f) * 18.0f;

    // Direction control: invert rotation direction if enabled
    const float directionMultiplier = effectDirectionInverted ? -1.0f : 1.0f;

    // Thickness control: map 0-255 to distance multiplier (INVERTED for intuitive control)
    // Higher values = thicker/wider bands (fewer bands in same space)
    // Remapped range: effective range is like 4-15 instead of 1-10
    // 0 = thin circles (2.0), 128 = medium (1.1), 255 = very thick/wide circles (0.2)
    const float thicknessFactor = 2.0f - (dualSpiralThickness / 255.0f) * 1.8f;

    // Create concentric circles using distance and time for rotation effect
    // We use time to create a rotating/expanding effect
    float circleValue = distance * thicknessFactor - cachedTime * rotationSpeed * directionMultiplier;

    // Create bands using sine wave - gives the alternating pattern
    float bands = sinf(circleValue);

    // Threshold to create sharp edges between colors
    const float threshold = 0.0f;

    if (bands > threshold) {
        // Use Color 1 (circle primary color)
        r = (uint8_t)(dualCircleR * intensity);
        g = (uint8_t)(dualCircleG * intensity);
        b = (uint8_t)(dualCircleB * intensity);
    } else {
        // Use Color 2 (gradient bottom / secondary color)
        r = (uint8_t)(gradientBottomR * intensity);
        g = (uint8_t)(gradientBottomG * intensity);
        b = (uint8_t)(gradientBottomB * intensity);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================
void ColorEffects::hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    int i = (int)h;
    float f = h - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    float rf, gf, bf;
    switch (i % 6) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        default: rf = v; gf = p; bf = q; break;
    }

    // uint8_t cast automatically clamps to 0-255, no need for constrain()
    r = (uint8_t)(rf * 255.0f);
    g = (uint8_t)(gf * 255.0f);
    b = (uint8_t)(bf * 255.0f);
}

bool ColorEffects::isInEyeRegion(int row, int col) const {
    // Check if row is within eye height range
    if (row < eyeOffsetY || row >= (eyeOffsetY + eyeHeight)) {
        return false;
    }

    // Check left eye region
    if (col >= eyeOffsetX && col < (eyeOffsetX + eyeWidth)) {
        return true;
    }

    // Check right eye region (mirrored on the right side)
    // Right eye starts at: screenWidth - eyeOffsetX - eyeWidth
    const int rightEyeOffsetX = screenWidth - eyeOffsetX - eyeWidth;
    if (col >= rightEyeOffsetX && col < (rightEyeOffsetX + eyeWidth)) {
        return true;
    }

    return false;
}

