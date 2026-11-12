#include "Hub75DMA.h"
#include "KMMXController/MotionDetectionConfig.h"

Hub75DMA::Hub75DMA() {
    HUB75_I2S_CFG::i2s_pins _pins = {R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
    HUB75_I2S_CFG mxconfig(panelWidth, panelHeight, panelsNumber, _pins);
    mxconfig.min_refresh_rate = minRefreshRate;
    mxconfig.double_buff = doubleBuffer;
    mxconfig.latch_blanking = 2;  // Preserve the panel timing used before HUB75 3.1.0.
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
    ready = matrix->begin() && matrix->calculated_refresh_rate > 0;
    if (!ready) { Serial.println("Display initialization failed"); return; }
    matrix->setBrightness8(panelBrightness);
    matrix->clearScreen();
}

int Hub75DMA::getRefreshRate() {
    return ready ? matrix->calculated_refresh_rate : 0;
}

void Hub75DMA::flipDMABuffer() {
    if (!ready) return;
    matrix->flipDMABuffer();
}

void Hub75DMA::clearScreen() {
    if (!ready) return;
    matrix->clearScreen();
}

void Hub75DMA::updateColorEffectsFrame() {
    colorEffects.updateFrame();
}

int Hub75DMA::getBrightnessValue() {
    return panelBrightness;
}

uint16_t Hub75DMA::color565(uint8_t r, uint8_t g, uint8_t b) {
    return matrix->color565(r, g, b);
}

void Hub75DMA::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (!ready) return;
    matrix->drawPixel(x, y, color);
}

void Hub75DMA::setBrightnessValue(uint8_t value) {
    panelBrightness = constrain(value, 0, 255);
    if (ready) matrix->setBrightness8(panelBrightness);
}

void Hub75DMA::setColorMode(uint8_t mode) {
    // Valid modes: 0 = gradient (customizable), 1 = spiral, 2 = plasma, 3 = radial pulse, 4 = dual spiral, 5 = dual circle
    if (mode <= 5) {
        colorMode = mode;
    }
}

uint8_t Hub75DMA::getColorMode() const {
    return colorMode;
}

void Hub75DMA::setGradientColors(uint8_t topR, uint8_t topG, uint8_t topB, uint8_t bottomR, uint8_t bottomG, uint8_t bottomB) {
    colorEffects.setGradientColors(topR, topG, topB, bottomR, bottomG, bottomB);
}

void Hub75DMA::getGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    colorEffects.getGradientTopColor(r, g, b);
}

void Hub75DMA::getGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    colorEffects.getGradientBottomColor(r, g, b);
}

void Hub75DMA::setDualSpiralColor(uint8_t spiralR, uint8_t spiralG, uint8_t spiralB) {
    colorEffects.setDualSpiralColor(spiralR, spiralG, spiralB);
}

void Hub75DMA::setDualCircleColor(uint8_t circleR, uint8_t circleG, uint8_t circleB) {
    colorEffects.setDualCircleColor(circleR, circleG, circleB);
}

void Hub75DMA::setEffectThickness(uint8_t thickness) {
    colorEffects.setEffectThickness(thickness);
}

uint8_t Hub75DMA::getEffectThickness() const {
    return colorEffects.getEffectThickness();
}

void Hub75DMA::setEffectSpeed(uint8_t speed) {
    colorEffects.setEffectSpeed(speed);
}

uint8_t Hub75DMA::getEffectSpeed() const {
    return colorEffects.getEffectSpeed();
}

void Hub75DMA::setEffectDirectionInverted(uint8_t inverted) {
    colorEffects.setEffectDirectionInverted(inverted);
}

uint8_t Hub75DMA::getEffectDirectionInverted() const {
    return colorEffects.getEffectDirectionInverted();
}

void Hub75DMA::getColorMap(const uint8_t lightness, const int row, const int col, uint8_t& r, uint8_t& g, uint8_t& b) {
    colorEffects.getColor(colorMode, lightness, row, col, r, g, b);
}

void Hub75DMA::updateGlitchParams(const IntensityCache& cache, bool isInitialTrigger) {
    // Determine glitch coverage area with intensity-based scaling
    int fullScreenChance = tapGlitchFullScreenChance + (int)(30 * cache.squared);

    if ((esp_random() % 100) < fullScreenChance) {
        glitchState.isFullScreen = true;
        glitchState.glitchRadius = 999;  // Keep for backward compatibility
    } else {
        glitchState.isFullScreen = false;
        const int minRadiusRange = 1;
        const int maxRadiusRange = 6;
        int radiusRange = maxRadiusRange - minRadiusRange;
        int scaledRadius = minRadiusRange + (int)(radiusRange * cache.squared);
        glitchState.glitchRadius = minRadiusRange + (esp_random() % (scaledRadius - minRadiusRange + 1));
    }

    // Calculate active glitch rows with cubic scaling, respecting maxRowLimit (2-8 rows)
    const int minActiveRows = 1;
    const int maxRowScaling = glitchState.maxRowLimit - minActiveRows;
    int activeRows = minActiveRows + (int)(cache.cubed * maxRowScaling);
    glitchState.activeRowCount = constrain(activeRows, minActiveRows, glitchState.maxRowLimit);

    // Shift constants (different for initial trigger vs updates)
    // Initial: 10-40px, Update: 15-80px with quartic scaling
    const int minHorizontalShift = isInitialTrigger ? 10 : 15;
    const int maxHorizontalShift = isInitialTrigger ? 40 : 80;
    const int minVerticalJitter = 0;
    const int maxVerticalJitter = 4;

    // Update active glitch rows
    for (uint8_t i = 0; i < glitchState.activeRowCount; i++) {
        glitchState.glitchRow[i] = esp_random() % panelHeight;

        // Horizontal shift with quartic scaling (10-40px initial, 15-80px update)
        int shiftRange = maxHorizontalShift - minHorizontalShift;
        int scaledShift = minHorizontalShift + (int)(shiftRange * cache.quad);
        glitchState.glitchShift[i] = (esp_random() % (scaledShift * 2 + 1)) - scaledShift;

        // Vertical jitter with quadratic scaling (0-4px)
        int jitterRange = maxVerticalJitter - minVerticalJitter;
        int scaledJitter = minVerticalJitter + (int)(jitterRange * cache.squared);
        glitchState.verticalJitter[i] = scaledJitter > 0 ? (esp_random() % (scaledJitter * 2 + 1)) - scaledJitter : 0;
    }

    // Mark remaining rows as inactive (only set glitchRow, others don't matter)
    for (uint8_t i = glitchState.activeRowCount; i < GlitchState::MAX_GLITCH_ROWS; i++) {
        glitchState.glitchRow[i] = -1;
    }
}

void Hub75DMA::triggerGlitch(int intensity) {
    glitchState.active = true;
    glitchState.startTime = millis();
    glitchState.lastUpdate = millis();
    glitchState.baseIntensity = constrain(intensity, 0, 100);
    glitchState.currentIntensity = 0.0f;  // Start at 0 for ramp-up effect

    // Calculate intensity power curves once
    calculateIntensityCurves(glitchState.baseIntensity, intensityCache);

    // Calculate effect duration with quadratic scaling
    glitchState.duration = tapGlitchMinDuration +
                          (unsigned long)(intensityCache.squared * (tapGlitchMaxDuration - tapGlitchMinDuration));

    // Set maximum row limit based on intensity (linear scaling from 1-8 rows)
    // Low intensity (0-12.5%) = 1 row, High intensity (87.5-100%) = 8 rows
    glitchState.maxRowLimit = tapGlitchMinRows + (uint8_t)(intensityCache.normalized * (tapGlitchMaxRows - tapGlitchMinRows));
    glitchState.maxRowLimit = constrain(glitchState.maxRowLimit, tapGlitchMinRows, tapGlitchMaxRows);

    // Initialize glitch parameters
    updateGlitchParams(intensityCache, true);
}

void Hub75DMA::updateGlitch() {
    if (!glitchState.active) return;

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - glitchState.startTime;

    // Check if glitch duration expired
    if (elapsed >= glitchState.duration) {
        glitchState.active = false;
        glitchState.isFullScreen = false;
        glitchState.currentIntensity = 1.0f;
        glitchState.maxRowLimit = tapGlitchMaxRows;

        // Only reset active rows (optimization: no need to clear all)
        for (uint8_t i = 0; i < glitchState.activeRowCount; i++) {
            glitchState.glitchRow[i] = -1;
        }
        glitchState.activeRowCount = 0;
        glitchState.glitchRadius = 2;
        return;
    }

    // Apply smooth ramp-up at start (reduces jarring effect)
    if (elapsed < tapGlitchRampDuration) {
        float rampProgress = (float)elapsed / tapGlitchRampDuration;
        glitchState.currentIntensity = rampProgress;  // Smooth 0.0 -> 1.0
    }
    // Apply smooth fade-out during final portion of duration
    else {
        const float fadeStartRatio = 0.8f;
        float fadeThreshold = glitchState.duration * fadeStartRatio;

        if (elapsed > fadeThreshold) {
            float fadeProgress = (elapsed - fadeThreshold) / (glitchState.duration - fadeThreshold);
            glitchState.currentIntensity = 1.0f - fadeProgress;
        } else {
            glitchState.currentIntensity = 1.0f;
        }
    }

    // Periodically randomize glitch parameters
    if (currentTime - glitchState.lastUpdate >= tapGlitchUpdateInterval) {
        glitchState.lastUpdate = currentTime;

        // Calculate effective intensity with ramp/fade applied and cache power curves
        float effectiveIntensity = glitchState.baseIntensity * glitchState.currentIntensity;
        IntensityCache cache;
        calculateIntensityCurves(effectiveIntensity, cache);

        // Update glitch parameters using shared helper
        updateGlitchParams(cache, false);
    }
}

bool Hub75DMA::isGlitchActive() const {
    return glitchState.active;
}
