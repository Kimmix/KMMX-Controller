#pragma once
#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Renderer/FlyingHeart.h"

// Forward declaration to avoid circular dependency
class EyeState;

enum class FXStateEnum { IDLE, Heart, Blush };

class FXState {
   private:
    Hub75DMA* display;
    EyeState* eyeState;
    FXStateEnum currentState = FXStateEnum::IDLE;
    FlyingHeart flyingHeart;

    unsigned long resetHeart;

    void handleAutoStateChanges();

   public:
    FXState(Hub75DMA* displayPtr = nullptr, EyeState* eyeStatePtr = nullptr);

    void update();
    void setState(FXStateEnum newState);
    FXStateEnum getState() const;
    void flyingHeartState();
    void blushingState();
    void setFlyingSpeed(float i);
    void setEyeStatePtr(EyeState* eyeStatePtr);
};
