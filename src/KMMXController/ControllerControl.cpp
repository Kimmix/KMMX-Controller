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
    if (renderRunning.load() && queueControl(&KMMXController::setCheekBrightness, i)) return;  // no render task: apply directly
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
// BLE value -> state; index 0 and out-of-range fall back to IDLE
static const EyeStateEnum eyeStates[] = {
    EyeStateEnum::IDLE, EyeStateEnum::GOOGLY, EyeStateEnum::HEART, EyeStateEnum::SMILE, EyeStateEnum::ANGRY,
    EyeStateEnum::SAD, EyeStateEnum::ARROW, EyeStateEnum::OEYE, EyeStateEnum::CRY, EyeStateEnum::DOUBTED,
    EyeStateEnum::ROUNDED, EyeStateEnum::SHARP, EyeStateEnum::GIGGLE, EyeStateEnum::UNIMPRESSED};
static const MouthStateEnum mouthStates[] = {
    MouthStateEnum::IDLE, MouthStateEnum::WAH, MouthStateEnum::EH, MouthStateEnum::POUT, MouthStateEnum::DROOLING,
    MouthStateEnum::ANGRY, MouthStateEnum::LOWER, MouthStateEnum::SHOCK, MouthStateEnum::SMALL, MouthStateEnum::WORRY};

void KMMXController::setEye(int i) {
    if (queueControl(&KMMXController::setEye, i)) return;
    const bool known = i > 0 && i < static_cast<int>(sizeof(eyeStates) / sizeof(eyeStates[0]));
    eyeState.setState(known ? eyeStates[i] : EyeStateEnum::IDLE, true, 0);  // Persistent, no timeout
    if (!known) eyeState.savePrevState(EyeStateEnum::IDLE);  // Reset restore point to IDLE for clean slate
}

void KMMXController::setMouth(int i) {
    if (queueControl(&KMMXController::setMouth, i)) return;
    const bool known = i > 0 && i < static_cast<int>(sizeof(mouthStates) / sizeof(mouthStates[0]));
    mouthState.setState(known ? mouthStates[i] : MouthStateEnum::IDLE, true, 0);  // Persistent, no timeout
    if (!known) mouthState.savePrevState(MouthStateEnum::IDLE);  // Reset restore point to IDLE for clean slate
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
