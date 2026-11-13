#include "SlotMachine.h"

#include <cassert>
#include <esp_system.h>

namespace {
const uint8_t symbols[][5] PROGMEM = {
    {0b000111, 0b001011, 0b010110, 0b101100, 0b111000},  // Diagonal PC RAM
    {0b01010, 0b11111, 0b11111, 0b01110, 0b00100},  // Heart
    {0b00100, 0b10101, 0b01110, 0b10101, 0b00100},  // Star
    {0b00100, 0b01110, 0b11111, 0b01110, 0b00100},  // Diamond
};
const uint8_t ramContacts[] PROGMEM = {0b000001, 0b000001, 0b000010, 0b000100, 0b001000};

const uint8_t glyphB[] PROGMEM = {0b110, 0b101, 0b110, 0b101, 0b110};
const uint8_t glyphA[] PROGMEM = {0b010, 0b101, 0b111, 0b101, 0b101};
const uint8_t glyphC[] PROGMEM = {0b011, 0b100, 0b100, 0b100, 0b011};
const uint8_t glyphD[] PROGMEM = {0b110, 0b101, 0b101, 0b101, 0b110};
const uint8_t glyphE[] PROGMEM = {0b111, 0b100, 0b110, 0b100, 0b111};
const uint8_t glyphJ[] PROGMEM = {0b001, 0b001, 0b001, 0b101, 0b010};
const uint8_t glyphK[] PROGMEM = {0b101, 0b110, 0b100, 0b110, 0b101};
const uint8_t glyphG[] PROGMEM = {0b011, 0b100, 0b101, 0b101, 0b011};
const uint8_t glyphM[] PROGMEM = {0b101, 0b111, 0b111, 0b101, 0b101};
const uint8_t glyphI[] PROGMEM = {0b111, 0b010, 0b010, 0b010, 0b111};
const uint8_t glyphO[] PROGMEM = {0b111, 0b101, 0b101, 0b101, 0b111};
const uint8_t glyphP[] PROGMEM = {0b110, 0b101, 0b110, 0b100, 0b100};
const uint8_t glyphT[] PROGMEM = {0b111, 0b010, 0b010, 0b010, 0b010};
const uint8_t glyphN[] PROGMEM = {0b101, 0b111, 0b111, 0b111, 0b101};
const uint8_t glyphW[] PROGMEM = {0b101, 0b101, 0b101, 0b111, 0b101};
const uint8_t glyphBang[] PROGMEM = {0b010, 0b010, 0b010, 0b000, 0b010};
const uint8_t glyphSpace[] PROGMEM = {0, 0, 0, 0, 0};

const uint8_t* const boopWord[] = {glyphB, glyphO, glyphO, glyphP, glyphBang};
const uint8_t* const jackpotWord[] = {glyphJ, glyphA, glyphC, glyphK, glyphP, glyphO, glyphT, glyphBang};
const uint8_t* const megaWord[] = {glyphM, glyphE, glyphG, glyphA, glyphBang};
const uint8_t* const awDangItWord[] = {
    glyphA, glyphW, glyphSpace, glyphD, glyphA, glyphN, glyphG, glyphSpace, glyphI, glyphT};
const uint32_t symbolColors[] = {0x45E06F, 0xFF4C8B, 0xFFD83D, 0x52C7FF};
const uint32_t rainbowColors[] = {0xFF3030, 0xFF9D2E, 0xFFE13D, 0x45E06F, 0x52C7FF, 0xB060FF};
const unsigned long reelStopMs[] = {700, 1000, 1300};
const int8_t particleDx[] = {-2, -1, 0, 1, 2, -2, 2, -1, 0, 1, -2, 2};
const int8_t particleDy[] = {-1, -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 1};
constexpr unsigned long reelBounceMs = 160;
constexpr unsigned long anticipationMs = 600;
constexpr int slotScreenOffsetX = 14;

constexpr int reelBounceOffset(unsigned long elapsed) {
    return elapsed >= reelBounceMs ? 0 : elapsed < reelBounceMs / 2
                                               ? elapsed * 2 / (reelBounceMs / 2)
                                               : (reelBounceMs - elapsed) * 2 / (reelBounceMs / 2);
}

constexpr unsigned long reelStepInterval(unsigned long elapsed, unsigned long stopAt) {
    return 45 + (elapsed < stopAt ? elapsed : stopAt) * 95 / stopAt;
}

constexpr uint8_t easedCharge(uint8_t value) {
    const uint16_t remaining = 100 - value;
    return 100 - remaining * remaining * remaining / 10000;
}

constexpr uint8_t smoothedCharge(uint8_t current, uint8_t target) {
    if (current < target) return current + (target - current + 3) / 4;
    if (current > target) return current - (current - target + 3) / 4;
    return current;
}

uint8_t matchingReelMask(const uint8_t* values, uint8_t stopped) {
    if (!stopped) return 0;
    uint8_t mask = 1;
    for (uint8_t i = 1; i < stopped; ++i) {
        if (values[i] == values[0]) mask |= 1 << i;
    }
    return mask;
}
}  // namespace

SlotMachine::SlotMachine(Hub75DMA* display) : display(display) {
#ifndef NDEBUG
    const uint8_t win[] = {1, 1, 1};
    const uint8_t loss[] = {1, 2, 1};
    assert(isWinning(win));
    assert(!isWinning(loss));
    assert(matchingReelMask(win, 3) == 0b111 && matchingReelMask(loss, 3) == 0b101);
    assert(reelBounceOffset(0) == 0 && reelBounceOffset(reelBounceMs / 2) == 2 && reelBounceOffset(reelBounceMs) == 0);
    assert(reelStepInterval(0, 1000) == 45 && reelStepInterval(1000, 1000) == 140);
    assert(easedCharge(0) == 0 && easedCharge(50) == 88 && easedCharge(100) == 100);
    assert(smoothedCharge(0, 100) == 25 && smoothedCharge(100, 0) == 75 && smoothedCharge(99, 100) == 100);
#endif
}

void SlotMachine::setEnabled(bool value) {
    enabled.store(value);
    if (!value) {
        spinRequested.store(false);
        outcome.store(Outcome::NONE);
    }
}

bool SlotMachine::isEnabled() const {
    return enabled.load();
}

SlotMachine::Outcome SlotMachine::getOutcome() const {
    return outcome.load();
}

SlotMachine::State SlotMachine::getState() const {
    return phase.load();
}

uint8_t SlotMachine::getCharge() const {
    return displayedCharge.load();
}

uint8_t SlotMachine::getStoppedReels() const {
    return stoppedReels.load();
}

uint8_t SlotMachine::getMatchingReels() const {
    return matchingReels.load();
}

bool SlotMachine::isAnticipating() const {
    return anticipating.load();
}

uint8_t SlotMachine::getResultSymbol() const {
    return resultSymbol.load();
}

void SlotMachine::update(uint16_t proximity, unsigned long now) {
    const bool active = isEnabled();
    if (active != inputWasEnabled) {
        boop.reset();
        inputWasEnabled = active;
    }
    if (!active) {
        charge.store(0);
        return;
    }
    const BoopResult boopResult = boop.update(proximity, now);
    if (boopResult.event == BoopEvent::APPROACHING) {
        const uint8_t linearCharge = (proximity - boopMinThreshold) * 100 / (boopMaxThreshold - boopMinThreshold);
        charge.store(easedCharge(linearCharge));
    } else if (boopResult.event == BoopEvent::COMPLETED) {
        charge.store(100);
        spinRequested.store(true);
    } else if (boopResult.event != BoopEvent::HELD) {
        charge.store(0);
    }
}

void SlotMachine::render(unsigned long now) {
    const bool active = isEnabled();
    if (!active) {
        renderWasEnabled = false;
        return;
    }
    if (!renderWasEnabled) {
        resetGame();
        renderWasEnabled = true;
    }
    if (now - lastChargeUpdateAt >= chargeSmoothingIntervalMs) {
        lastChargeUpdateAt = now;
        displayedCharge.store(smoothedCharge(displayedCharge.load(), charge.load()));
    }
    if (spinRequested.exchange(false) && phase != State::SPINNING && phase != State::REVEAL) startSpin(now);
    if (phase == State::SPINNING) updateSpin(now);
    if (phase == State::REVEAL && now - phaseStartedAt >= revealDurationMs) {
        outcome.store(winning ? Outcome::WIN : Outcome::LOSE);
        phase = State::RESULT;
        phaseStartedAt = now;
    }
    const unsigned long resultTime = winning && reels[0] == 3 ? megaResultDurationMs : resultDurationMs;
    if (phase == State::RESULT && now - phaseStartedAt >= resultTime) {
        phase = State::READY;
        outcome.store(Outcome::NONE);
    }
    drawPanel(now);
}

void SlotMachine::startSpin(unsigned long now) {
    for (uint8_t& value : result) value = esp_random() % symbolCount;
    for (uint8_t i = 0; i < reelCount; ++i) {
        previousReels[i] = reels[i];
        reels[i] = esp_random() % symbolCount;
    }
    phase = State::SPINNING;
    phaseStartedAt = now;
    for (unsigned long& stepAt : lastReelStepAt) stepAt = now;
    winning = false;
    charge.store(0);
    displayedCharge.store(0);
    stoppedReels.store(0);
    matchingReels.store(0);
    anticipating.store(false);
    outcome.store(Outcome::NONE);
}

void SlotMachine::updateSpin(unsigned long now) {
    const unsigned long elapsed = now - phaseStartedAt;
    const bool matchingPair = result[0] == result[1];
    uint8_t stopped = 0;
    for (uint8_t i = 0; i < reelCount; ++i) {
        const unsigned long stopAt = reelStopMs[i] + (i == reelCount - 1 && matchingPair ? anticipationMs : 0);
        if (elapsed >= stopAt) {
            reels[i] = result[i];
            ++stopped;
        } else if (now - lastReelStepAt[i] >= reelStepInterval(elapsed, stopAt)) {
            lastReelStepAt[i] = now;
            previousReels[i] = reels[i];
            reels[i] = esp_random() % symbolCount;
        }
    }
    stoppedReels.store(stopped);
    matchingReels.store(matchingReelMask(result, stopped));
    anticipating.store(matchingPair && elapsed >= reelStopMs[1] && elapsed < reelStopMs[2] + anticipationMs);
    const unsigned long finalStopAt = reelStopMs[reelCount - 1] + (matchingPair ? anticipationMs : 0);
    if (elapsed >= finalStopAt + reelBounceMs) {
        for (uint8_t i = 0; i < reelCount; ++i) reels[i] = result[i];
        winning = isWinning(reels);
        resultSymbol.store(reels[0]);
        anticipating.store(false);
        phase = State::REVEAL;
        phaseStartedAt = now;
    }
}

void SlotMachine::resetGame() {
    phase = State::READY;
    winning = false;
    charge.store(0);
    displayedCharge.store(0);
    stoppedReels.store(0);
    matchingReels.store(0);
    anticipating.store(false);
    outcome.store(Outcome::NONE);
    reels[0] = 0;
    reels[1] = 1;
    reels[2] = 2;
    for (uint8_t i = 0; i < reelCount; ++i) previousReels[i] = reels[i];
    spinRequested.store(false);
}

void SlotMachine::drawPanel(unsigned long now) {
    const uint16_t white = display->color565(210, 210, 210);
    const uint16_t yellow = display->color565(255, 210, 35);
    const uint16_t red = display->color565(180, 30, 30);
    const uint16_t pink = display->color565(255, 55, 155);
    const uint32_t themeRgb = symbolColors[result[0] % symbolCount];
    const uint16_t theme = display->color565(themeRgb >> 16, themeRgb >> 8, themeRgb);
    const unsigned long spinElapsed = now - phaseStartedAt;
    const bool anticipating = phase == State::SPINNING && result[0] == result[1] &&
                              spinElapsed >= reelStopMs[1] && spinElapsed < reelStopMs[2] + anticipationMs;

    if (phase == State::READY) {
        drawWord(boopWord, 5, 16, 1, white);
    }
    if (phase == State::RESULT) {
        if (winning && reels[0] == 3) {
            const uint32_t rgb = rainbowColors[(now / 90) % 6];
            drawWord(megaWord, 5, 16, 1, display->color565(rgb >> 16, rgb >> 8, rgb));
        } else if (winning) drawWord(jackpotWord, 8, 10, 1, theme);
        else drawWord(awDangItWord, 10, 5, 1, red);
    }
    if (phase == State::READY || phase == State::RESULT) drawChargeMeter(getCharge(), yellow);

    const uint16_t frameColor = phase == State::RESULT && winning
                                    ? ((now / 150) % 2 ? theme : white)
                                    : (phase == State::RESULT ? red : (anticipating ? ((now / 90) % 2 ? yellow : pink) : white));
    for (uint8_t i = 0; i < reelCount; ++i) {
        const int x = i * 17;
        drawRect(x, 8, 16, 18, frameColor);
        if (phase == State::SPINNING) {
            const unsigned long stopAt = reelStopMs[i] + (i == reelCount - 1 && result[0] == result[1] ? anticipationMs : 0);
            if (spinElapsed < stopAt) {
                const unsigned long stepMs = reelStepInterval(spinElapsed, stopAt);
                const unsigned long stepElapsed = now - lastReelStepAt[i];
                const int offset = (stepElapsed < stepMs ? stepElapsed : stepMs - 1) * 10 / stepMs;
                drawSymbol(previousReels[i], x + 3, 12 + offset, 64);
                drawSymbol(reels[i], x + 3, 2 + offset);
            } else {
                const unsigned long stopAge = spinElapsed - stopAt;
                const bool squashed = stopAge < 70;
                drawSymbol(result[i], x + 3, squashed ? 14 : 12 + reelBounceOffset(stopAge), 255, -1, squashed);
                if (stopAge < 120 && (stopAge / 30) % 2 == 0) {
                    drawPixelBoth(x + 1, 15, theme);
                    drawPixelBoth(x + 14, 19, theme);
                }
            }
        } else {
            uint8_t brightness = 255;
            int highlightColumn = -1;
            if (phase == State::READY) {
                const uint8_t shimmer = (now / 25 + i * 10) % 40;
                brightness = 150 + (shimmer < 20 ? shimmer : 39 - shimmer) * 5;
            } else if (winning && reels[i] == 1) {
                const uint8_t pulse = (now / 20) % 32;
                brightness = 160 + (pulse < 16 ? pulse : 31 - pulse) * 6;
            } else if (winning && reels[i] == 3) {
                highlightColumn = (now / 65) % 14 - 2;
            }
            drawSymbol(reels[i], x + 3, 12, brightness, highlightColumn);
        }
    }
    if (phase == State::READY) drawMarquee(now, display->color565(45, 70, 100), display->color565(115, 45, 105));
    if (phase == State::RESULT && winning) {
        if (reels[0] == 3) drawRainbowMarquee(now);
        else drawMarquee(now, theme, white);
        drawWinParticles(now, theme, result[0] == 0 ? yellow : white);
    }
}

void SlotMachine::drawSymbol(uint8_t symbol, int x, int y, uint8_t brightness,
                             int highlightColumn, bool squashed) {
    const uint8_t symbolIndex = symbol % symbolCount;
    const uint8_t width = symbolIndex == 0 ? 6 : 5;
    const int originX = symbolIndex == 0 ? x - 1 : x;
    const uint32_t rgb = symbolColors[symbolIndex];
    const uint16_t color = display->color565(((rgb >> 16) & 0xFF) * brightness / 255,
                                             ((rgb >> 8) & 0xFF) * brightness / 255,
                                             (rgb & 0xFF) * brightness / 255);
    const uint16_t contacts = display->color565(255 * brightness / 255, 190 * brightness / 255,
                                                40 * brightness / 255);
    const uint16_t highlight = display->color565(255, 255, 255);
    for (uint8_t row = 0; row < 5; ++row) {
        const uint8_t bits = pgm_read_byte(&symbols[symbolIndex][row]);
        const uint8_t contactBits = symbolIndex == 0 ? pgm_read_byte(&ramContacts[row]) : 0;
        for (uint8_t col = 0; col < width; ++col) {
            const uint8_t bit = 1 << (width - 1 - col);
            if (!(bits & bit)) continue;
            const uint16_t pixelColor = contactBits & bit ? contacts : color;
            const uint8_t pixelHeight = squashed ? 1 : 2;
            for (uint8_t py = 0; py < pixelHeight; ++py) {
                const int pixelY = y + row * pixelHeight + py;
                if (pixelY < 9 || pixelY > 24) continue;
                drawPixelBoth(originX + col * 2, pixelY, col * 2 == highlightColumn ? highlight : pixelColor);
                drawPixelBoth(originX + col * 2 + 1, pixelY,
                              col * 2 + 1 == highlightColumn ? highlight : pixelColor);
            }
        }
    }
}

void SlotMachine::drawChargeMeter(uint8_t value, uint16_t color) {
    const uint16_t dim = display->color565(45, 45, 45);
    drawRect(0, 28, 50, 4, dim);
    const int filled = value * 48 / 100;
    for (int x = 1; x <= filled; ++x) {
        drawPixelBoth(x, 29, color);
        drawPixelBoth(x, 30, color);
    }
}

void SlotMachine::drawMarquee(unsigned long now, uint16_t first, uint16_t second) {
    const bool phase = (now / 120) % 2;
    for (int x = 1; x < 49; x += 4) {
        const uint16_t color = ((x / 4) % 2) == phase ? first : second;
        drawPixelBoth(x, 8, color);
        drawPixelBoth(x, 25, color);
    }
    for (int y = 9; y < 25; y += 4) {
        const uint16_t color = ((y / 4) % 2) == phase ? second : first;
        drawPixelBoth(0, y, color);
        drawPixelBoth(49, y, color);
    }
}

void SlotMachine::drawRainbowMarquee(unsigned long now) {
    const uint8_t offset = (now / 90) % 6;
    for (int x = 1; x < 49; x += 4) {
        const uint32_t rgb = rainbowColors[(x / 4 + offset) % 6];
        const uint16_t color = display->color565(rgb >> 16, rgb >> 8, rgb);
        drawPixelBoth(x, 8, color);
        drawPixelBoth(x, 25, color);
    }
    for (int y = 9; y < 25; y += 4) {
        const uint32_t rgb = rainbowColors[(y / 4 + offset) % 6];
        const uint16_t color = display->color565(rgb >> 16, rgb >> 8, rgb);
        drawPixelBoth(0, y, color);
        drawPixelBoth(49, y, color);
    }
}

void SlotMachine::drawWinParticles(unsigned long now, uint16_t first, uint16_t second) {
    const unsigned long elapsed = now - phaseStartedAt;
    if (elapsed < 700) {
        const int distance = elapsed / 70;
        for (uint8_t i = 0; i < 12; ++i) {
            drawPixelBoth(25 + particleDx[i] * distance, 17 + particleDy[i] * distance,
                          i % 2 ? first : second);
        }
        return;
    }

    const int fall = (elapsed - 700) / 90;
    for (uint8_t i = 0; i < 12; ++i) {
        const int x = (i * 13 + (i % 3) * fall) % 50;
        const int y = 7 + (i * 7 + fall) % 24;
        drawPixelBoth(x, y, i % 2 ? first : second);
        if (result[0] == 2 && i < 4) {
            drawPixelBoth(x - 1, y, second);
            drawPixelBoth(x + 1, y, second);
            drawPixelBoth(x, y - 1, second);
            drawPixelBoth(x, y + 1, second);
        }
    }
}

void SlotMachine::drawWord(const uint8_t* const* glyphs, uint8_t count, int x, int y, uint16_t color) {
    // Mirror the word's placement while keeping its letters readable on both panels.
    const int leftX = x + slotScreenOffsetX;
    const int rightX = screenWidth - leftX - (count * 4 - 1);
    for (uint8_t i = 0; i < count; ++i) {
        drawGlyph(glyphs[i], leftX + i * 4, y, color);
        drawGlyph(glyphs[i], rightX + i * 4, y, color);
    }
}

void SlotMachine::drawGlyph(const uint8_t* glyph, int x, int y, uint16_t color) {
    for (uint8_t row = 0; row < 5; ++row) {
        const uint8_t bits = pgm_read_byte(glyph + row);
        for (uint8_t col = 0; col < 3; ++col) {
            if (bits & (1 << (2 - col))) display->drawPixel(x + col, y + row, color);
        }
    }
}

void SlotMachine::drawRect(int x, int y, int width, int height, uint16_t color) {
    for (int px = x; px < x + width; ++px) {
        drawPixelBoth(px, y, color);
        drawPixelBoth(px, y + height - 1, color);
    }
    for (int py = y + 1; py < y + height - 1; ++py) {
        drawPixelBoth(x, py, color);
        drawPixelBoth(x + width - 1, py, color);
    }
}

void SlotMachine::drawPixelBoth(int x, int y, uint16_t color) {
    const int shiftedX = x + slotScreenOffsetX;
    display->drawPixel(shiftedX, y, color);
    display->drawPixel(screenWidth - 1 - shiftedX, y, color);
}

bool SlotMachine::isWinning(const uint8_t values[reelCount]) {
    return values[0] == values[1] && values[1] == values[2];
}
