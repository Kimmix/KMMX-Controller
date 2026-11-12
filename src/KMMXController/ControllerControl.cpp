#include "KMMXController.h"

// FPS Counter Access
float KMMXController::getFPS() const {
    return fpsCounter.getFPS();
}

int KMMXController::getTargetFPS() const {
    return fpsCounter.getTargetFPS();
}

int KMMXController::getHornBrightness() {
    return hornLED.getBrightness();
}

void KMMXController::setHornBrightness(int i) {
    hornLED.setBrightness(i, 20);  // Set brightness with fade speed of 20 for faster response
}

int KMMXController::getCheekBrightness() {
    // Map 0-255 internal range back to 0-100 for BLE interface
    return fastMap<int>(cheekPanel.getBrightness(), 0, 255, 0, 100);
}

void KMMXController::setCheekBrightness(int i) {
    // Map 0-100 input range to 0-255 range for NeoPixel brightness
    int mappedBrightness = fastMap<int>(constrain(i, 0, 100), 0, 100, 0, 255);
    cheekPanel.setBrightness(mappedBrightness);
}

void KMMXController::setCheekBackgroundColor(uint8_t r, uint8_t g, uint8_t b) {
    cheekPanel.setBackgroundColorRGB(r, g, b);
}

void KMMXController::setCheekFadeColor(uint8_t r, uint8_t g, uint8_t b) {
    cheekPanel.setFadeColorRGB(r, g, b);
}

uint32_t KMMXController::getCheekBackgroundColor() {
    return cheekPanel.getBackgroundColor();
}

uint32_t KMMXController::getCheekFadeColor() {
    return cheekPanel.getFadeColor();
}

// State control for BLE
void KMMXController::setEye(int i) {
    if (queueControl(&KMMXController::setEye, i)) return;
    switch (i) {
        case 1:
            eyeState.setState(EyeStateEnum::GOOGLY, true, 0);  // Persistent, no timeout
            break;
        case 2:
            eyeState.setState(EyeStateEnum::HEART, true, 0);  // Persistent, no timeout
            break;
        case 3:
            eyeState.setState(EyeStateEnum::SMILE, true, 0);  // Persistent, no timeout
            break;
        case 4:
            eyeState.setState(EyeStateEnum::ANGRY, true, 0);  // Persistent, no timeout
            break;
        case 5:
            eyeState.setState(EyeStateEnum::SAD, true, 0);  // Persistent, no timeout
            break;
        case 6:
            eyeState.setState(EyeStateEnum::ARROW, true, 0);  // Changed from BOOP to ARROW for BLE control, Persistent, no timeout
            break;
        case 7:
            eyeState.setState(EyeStateEnum::OEYE, true, 0);  // Persistent, no timeout
            break;
        case 8:
            eyeState.setState(EyeStateEnum::CRY, true, 0);  // Persistent, no timeout
            break;
        case 9:
            eyeState.setState(EyeStateEnum::DOUBTED, true, 0);  // Persistent, no timeout
            break;
        case 10:
            eyeState.setState(EyeStateEnum::ROUNDED, true, 0);  // Persistent, no timeout
            break;
        case 11:
            eyeState.setState(EyeStateEnum::SHARP, true, 0);  // Persistent, no timeout
            break;
        case 12:
            eyeState.setState(EyeStateEnum::GIGGLE, true, 0);  // Persistent, no timeout
            break;
        case 13:
            eyeState.setState(EyeStateEnum::UNIMPRESSED, true, 0);  // Persistent, no timeout
            break;
        default:
            eyeState.setState(EyeStateEnum::IDLE, true, 0);  // Persistent, no timeout
            eyeState.savePrevState(EyeStateEnum::IDLE);  // Reset restore point to IDLE for clean slate
            break;
    }
}

void KMMXController::setMouth(int i) {
    if (queueControl(&KMMXController::setMouth, i)) return;
    switch (i) {
        case 1:
            mouthState.setState(MouthStateEnum::WAH, true, 0);  // Persistent, no timeout
            break;
        case 2:
            mouthState.setState(MouthStateEnum::EH, true, 0);  // Persistent, no timeout
            break;
        case 3:
            mouthState.setState(MouthStateEnum::POUT, true, 0);  // Persistent, no timeout
            break;
        case 4:
            mouthState.setState(MouthStateEnum::DROOLING, true, 0);  // Persistent, no timeout
            break;
        case 5:
            mouthState.setState(MouthStateEnum::ANGRY, true, 0);  // Persistent, no timeout
            break;
        case 6:
            mouthState.setState(MouthStateEnum::LOWER, true, 0);  // Persistent, no timeout
            break;
        case 7:
            mouthState.setState(MouthStateEnum::SHOCK, true, 0);  // Persistent, no timeout
            break;
        case 8:
            mouthState.setState(MouthStateEnum::SMALL, true, 0);  // Persistent, no timeout
            break;
        case 9:
            mouthState.setState(MouthStateEnum::WORRY, true, 0);  // Persistent, no timeout
            break;
        default:
            mouthState.setState(MouthStateEnum::IDLE, true, 0);  // Persistent, no timeout
            mouthState.savePrevState(MouthStateEnum::IDLE);  // Reset restore point to IDLE for clean slate
            break;
    }
}

void KMMXController::setViseme(int b) {
    if (queueControl(&KMMXController::setViseme, b)) return;
    if (b == 0) {
        mouthState.setState(MouthStateEnum::IDLE, true, 0);  // Persistent, no timeout
        mouthState.savePrevState(MouthStateEnum::IDLE);  // Reset restore point to IDLE for clean slate
    } else if (b == 1) {
        mouthState.setState(MouthStateEnum::TALKING, true, 0);  // Persistent, no timeout
    }
}

int KMMXController::getViseme() {
    return mouthState.getState() == MouthStateEnum::TALKING;
}

// System Control
void KMMXController::reboot() {
    Serial.println("Rebooting device...");
    delay(100);  // Give time for serial message to be sent
    ESP.restart();
}

// Fan Control
#if HAS_FAN_CONTROL
void KMMXController::setFanSpeed(int speed) {
    fan.setSpeed(speed);
}

int KMMXController::getFanSpeed() {
    return fan.getSpeed();
}
#endif

bool KMMXController::queueControl(ControlHandler handler, int value) {
    if (xTaskGetCurrentTaskHandle() == renderTaskHandle) return false;
    if (renderRunning.load()) {
        const RenderCommand command{handler, value, {}};
        xQueueSend(commandQueue, &command, portMAX_DELAY);
    }
    return true;
}
