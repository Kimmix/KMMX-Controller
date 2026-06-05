#include "KMMXController.h"
#include "Network/BLE.h"
#include "MotionDetectionConfig.h"
extern BLEManager& bleManager;

void KMMXController::update() {
    statusLED.update();
    cheekPanel.update();
    hornLED.update();
    if (boopInitialized && enableBoopDetection) {
        handleBoop();
    }
    if (oledInitialized) {
        updateOLED();
    }
}

void KMMXController::renderFace() {
    display.flipDMABuffer();
    display.clearScreen();
    display.updateColorEffectsFrame();  // Update cached time once per frame
    display.drawNose(noseNew);
    mouthState.update();
    eyeState.update();
    fxState.update();
}

void KMMXController::renderTask(void* parameter) {
    KMMXController* ctrl = static_cast<KMMXController*>(parameter);

    // Calculate frame interval based on refresh rate
    int refreshRate = ctrl->display.getRefreshRate();
    int frameDelayMs = 1000 / refreshRate;

    // Initialize FPS counter with target rate
    ctrl->fpsCounter.setTargetFPS(refreshRate);

    while (true) {
        // Flip buffer to display the previously rendered frame
        ctrl->display.flipDMABuffer();

        // Wait for DMA to complete the buffer swap (this is critical!)
        vTaskDelay(pdMS_TO_TICKS(frameDelayMs));

        // Now render the next frame to the back buffer
        ctrl->display.clearScreen();
        ctrl->display.updateColorEffectsFrame();  // Update cached time once per frame
        ctrl->display.updateGlitch();  // Update glitch effect state
        ctrl->display.drawNose(noseNew);
        ctrl->mouthState.update();
        ctrl->eyeState.update();
        ctrl->fxState.update();

        // Update FPS counter
        ctrl->fpsCounter.update();
    }
}
