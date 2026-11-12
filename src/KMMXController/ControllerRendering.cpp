#include "KMMXController.h"
#include "Network/BLE.h"
#include "MotionDetectionConfig.h"
extern BLEManager& bleManager;

void KMMXController::update() {
    if (renderTaskHandle && xTaskGetCurrentTaskHandle() != renderTaskHandle) return;
    statusLED.update();
    updateSlotMachineCheeks();
    if (!slotMachine.isEnabled()) cheekPanel.update();
    updateSlotMachineHorn();
    hornLED.update();
    const bool gameActive = slotMachine.isEnabled();
    if (gameActive != slotMachineWasEnabled) {
        boop.reset();
        if (isSleeping) resetIdleTime();
        else {
            stillTime = 0;
            motionCounter = 0;
        }
        slotMachineWasEnabled = gameActive;
    }
    if (boopInitialized) {
        slotMachine.update(getSensorData().proximity);
        if (renderRunning.load() && !gameActive && enableBoopDetection) handleBoop();
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
    int frameDelayMs = max(1, 1000 / refreshRate);
    ctrl->renderRunning.store(true);

    // Initialize FPS counter with target rate
    ctrl->fpsCounter.setTargetFPS(refreshRate);

    while (true) {
        RenderCommand command;
        for (unsigned count = 0; count < commandCapacity && xQueueReceive(ctrl->commandQueue, &command, 0) == pdTRUE; ++count) {
            if (command.handler) (ctrl->*command.handler)(command.value);
            else ctrl->acceptSensor(command.sample);
        }
        ctrl->update();
        // Flip buffer to display the previously rendered frame
        ctrl->display.flipDMABuffer();

        // Wait for DMA to complete the buffer swap (this is critical!)
        vTaskDelay(pdMS_TO_TICKS(frameDelayMs));

        // Now render the next frame to the back buffer
        ctrl->display.clearScreen();
        ctrl->display.updateColorEffectsFrame();  // Update cached time once per frame
        ctrl->display.updateGlitch();  // Update glitch effect state
        if (ctrl->slotMachine.isEnabled()) {
            ctrl->slotMachine.render();
        } else {
            ctrl->display.drawNose(noseNew);
            ctrl->mouthState.update();
            ctrl->eyeState.update();
            ctrl->fxState.update();
        }

        // Update FPS counter
        ctrl->fpsCounter.update();
    }
}
