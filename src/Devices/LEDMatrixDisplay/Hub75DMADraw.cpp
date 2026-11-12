#include "Hub75DMA.h"
#include "config.h"
#include "Renderer/GooglyEyeConfig.h"

void Hub75DMA::drawFullscreen(const uint8_t* bitmap) {
    drawBitmap(bitmap, panelWidth, panelHeight, 0, 0);
}

void Hub75DMA::drawEyePupil(const uint8_t* bitmap, int x, int y) {
    drawBitmap(bitmap, googlyPupilWidth, googlyPupilHeight, googlyPupilOffsetX + x, googlyPupilOffsetY + y);
}

void Hub75DMA::drawNose(const uint8_t* bitmap) {
    drawBitmap(bitmap, noseWidth, noseHeight, noseOffsetX, noseOffsetY);
}

void Hub75DMA::drawEye(const uint8_t* bitmap) {
    lastEyeBitmap = bitmap;  // Track last drawn eye bitmap
    drawBitmap(bitmap, eyeWidth, eyeHeight, eyeOffsetX, eyeOffsetY);
}

void Hub75DMA::drawEye(const uint8_t* bitmapL, const uint8_t* bitmapR) {
    lastEyeBitmap = bitmapL;  // Track left eye bitmap (for mirroring)
    drawBitmap(bitmapL, bitmapR, eyeWidth, eyeHeight, eyeOffsetX, eyeOffsetY);
}

void Hub75DMA::drawMouth(const uint8_t* bitmap) {
    lastMouthBitmap = bitmap;  // Track last drawn mouth bitmap
    drawBitmap(bitmap, mouthWidth, mouthHeight, mouthOffsetX, mouthOffsetY);
}

void Hub75DMA::drawBitmap(const uint8_t* bitmap, int imageWidth, int imageHeight, int offsetX, int offsetY) {
    if (!ready) return;
    const int mirrorBaseX = -offsetX + panelWidth;
    const int screenWidth = panelWidth * 2;

    for (int i = 0; i < imageHeight; i++) {
        const int row = offsetY + i;
        const uint8_t* rowPtr = bitmap + i * imageWidth;

        // Apply glitch effects (combined getter for efficiency)
        int rowShift, verticalJitter;
        getGlitchEffects(row, rowShift, verticalJitter);
        const int adjustedRow = row + verticalJitter;

        if (adjustedRow < 0 || adjustedRow >= panelHeight) continue;

        for (int j = 0; j < imageWidth; j++) {
            const uint8_t pixel = pgm_read_byte(rowPtr + j);
            if (pixel <= minimumPixelBrightness) continue;

            uint8_t r, g, b;
            const int colLeft = offsetX + j + rowShift;
            const int colRight = mirrorBaseX + (panelWidth - 1 - j) + rowShift;

            if (colLeft >= 0 && colLeft < screenWidth) {
                getColorMap(pixel, row, colLeft, r, g, b);
                matrix->drawPixelRGB888(colLeft, adjustedRow, r, g, b);
            }

            if (colRight >= 0 && colRight < screenWidth) {
                getColorMap(pixel, row, colRight, r, g, b);
                matrix->drawPixelRGB888(colRight, adjustedRow, r, g, b);
            }
        }
    }
}

void Hub75DMA::drawBitmap(const uint8_t* bitmapL, const uint8_t* bitmapR, int imageWidth, int imageHeight, int offsetX, int offsetY) {
    if (!ready) return;
    const int mirrorBaseX = -offsetX + panelWidth;
    const int screenWidth = panelWidth * 2;

    for (int i = 0; i < imageHeight; i++) {
        const int row = offsetY + i;
        const uint8_t* rowPtrL = bitmapL + i * imageWidth;
        const uint8_t* rowPtrR = bitmapR + i * imageWidth;

        // Apply glitch effects (combined getter for efficiency)
        int rowShift, verticalJitter;
        getGlitchEffects(row, rowShift, verticalJitter);
        const int adjustedRow = row + verticalJitter;

        if (adjustedRow < 0 || adjustedRow >= panelHeight) continue;

        for (int j = 0; j < imageWidth; j++) {
            const uint8_t pixelL = pgm_read_byte(rowPtrL + j);
            const uint8_t pixelR = pgm_read_byte(rowPtrR + j);
            uint8_t r, g, b;

            if (pixelL > minimumPixelBrightness) {
                const int colLeft = offsetX + j + rowShift;
                if (colLeft >= 0 && colLeft < screenWidth) {
                    getColorMap(pixelL, row, colLeft, r, g, b);
                    matrix->drawPixelRGB888(colLeft, adjustedRow, r, g, b);
                }
            }

            if (pixelR > minimumPixelBrightness) {
                const int colRight = mirrorBaseX + (panelWidth - 1 - j) + rowShift;
                if (colRight >= 0 && colRight < screenWidth) {
                    getColorMap(pixelR, row, colRight, r, g, b);
                    matrix->drawPixelRGB888(colRight, adjustedRow, r, g, b);
                }
            }
        }
    }
}

void Hub75DMA::drawBitmap(const uint8_t* bitmap, int imageWidth, int imageHeight, int offsetX, int offsetY, uint8_t r, uint8_t g, uint8_t b) {
    if (!ready) return;
    for (int i = 0; i < imageHeight; i++) {
        const int row = offsetY + i;
        const uint8_t* rowPtr = bitmap + i * imageWidth;

        for (int j = 0; j < imageWidth; j++) {
            const uint8_t pixel = pgm_read_byte(rowPtr + j);
            if (pixel > minimumPixelBrightness) {
                matrix->drawPixelRGB888(offsetX + j, row, r, g, b);
            }
        }
    }
}


