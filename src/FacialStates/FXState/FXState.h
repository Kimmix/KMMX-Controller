#pragma once
#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Renderer/FlyingHeart.h"
#include "Renderer/WeepingTears.h"

// Forward declaration to avoid circular dependency
class EyeState;

enum class FXStateEnum { IDLE, Heart, Blush, Tears };

class FXState {
   private:
    Hub75DMA* display;
    EyeState* eyeState;
    FXStateEnum currentState = FXStateEnum::IDLE;
    FlyingHeart flyingHeart;
    WeepingTears weepingTears;

    unsigned long resetHeart;
    unsigned long resetTears;

    void handleAutoStateChanges();

   public:
    FXState(Hub75DMA* displayPtr = nullptr, EyeState* eyeStatePtr = nullptr);

    void update();
    void setState(FXStateEnum newState);
    FXStateEnum getState() const;
    void flyingHeartState();
    void blushingState();
    void weepingTearsState();
    void setFlyingSpeed(float i);
    void setTearIntensity(float i);
    void setEyeStatePtr(EyeState* eyeStatePtr);
};
