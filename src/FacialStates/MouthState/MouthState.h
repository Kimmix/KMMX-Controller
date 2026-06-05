#pragma once
#include <Adafruit_Sensor.h>
#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Bitmaps/Bitmaps.h"
#include "Utils/Utils.h"
#include "Renderer/Viseme.h"
#include "Renderer/TimeBasedAnimation.h"
#include "Types/SensorData.h"

enum class MouthStateEnum { IDLE,
                            BOOP,
                            ANGRYBOOP,
                            TALKING,
                            WAH,
                            EH,
                            POUT,
                            DROOLING };

// Animation data structure for mouth animations with transition + loop pattern
struct MouthAnimationData {
    const uint8_t** frames;             // Array of frame pointers
    uint8_t frameCount;                 // Total number of frames
    TimeBasedAnimState transitionAnim;  // Full animation (plays once during transition)
    TimeBasedAnimState loopAnim;        // Loop animation (subset of frames, loops continuously)
    uint8_t loopFrameCount;             // Number of frames to use for looping
};

class MouthState {
   public:
    MouthState(Hub75DMA* display);
    Viseme viseme;
    void startMic();
    void update();
    void setState(MouthStateEnum newState, bool isPersistent = false, unsigned long durationMs = 0);
    void savePrevState(MouthStateEnum newState);
    void playPrevState();
    MouthStateEnum getState() const;
    void setSensorData(const SensorData& data);
    void resetMovingMouth();
    void setSlowAnimation(bool slow);

   private:
    Hub75DMA* display;
    SensorData sensorData;
    MouthStateEnum prevState, currentState = MouthStateEnum::IDLE;
    unsigned long mouthInterval, nextAngry;
    unsigned long stateStartTime = 0;       // When current state started (for auto-reset)
    unsigned long customResetDuration = 0;  // Custom duration override (0 = infinite)
    bool isTransitioning = false;
    const uint8_t *visemeFrame = mouthDefault, *mouthFrame = mouthDefault;

    const uint8_t* defaultAnimation[60] = {mouthDefault1, mouthDefault2, mouthDefault3, mouthDefault4, mouthDefault5, mouthDefault6, mouthDefault7, mouthDefault8, mouthDefault9, mouthDefault10, mouthDefault11, mouthDefault12, mouthDefault13, mouthDefault14, mouthDefault15, mouthDefault16, mouthDefault17, mouthDefault18, mouthDefault19, mouthDefault20, mouthDefault21, mouthDefault22, mouthDefault23, mouthDefault24, mouthDefault25, mouthDefault26, mouthDefault27, mouthDefault28, mouthDefault29, mouthDefault30, mouthDefault31, mouthDefault32, mouthDefault33, mouthDefault34, mouthDefault35, mouthDefault36, mouthDefault37, mouthDefault38, mouthDefault39, mouthDefault40, mouthDefault41, mouthDefault42, mouthDefault43, mouthDefault44, mouthDefault45, mouthDefault46, mouthDefault47, mouthDefault48, mouthDefault49, mouthDefault50, mouthDefault51, mouthDefault52, mouthDefault53, mouthDefault54, mouthDefault55, mouthDefault56, mouthDefault57, mouthDefault58, mouthDefault59, mouthDefault60};
    uint8_t defaultAnimationLength = arrayLength(defaultAnimation);

    const uint8_t* mouthUp[60] = {mouthUp1, mouthUp2, mouthUp3, mouthUp4, mouthUp5, mouthUp6, mouthUp7, mouthUp8, mouthUp9, mouthUp10, mouthUp11, mouthUp12, mouthUp13, mouthUp14, mouthUp15, mouthUp16, mouthUp17, mouthUp18, mouthUp19, mouthUp20, mouthUp21, mouthUp22, mouthUp23, mouthUp24, mouthUp25, mouthUp26, mouthUp27, mouthUp28, mouthUp29, mouthUp30, mouthUp31, mouthUp32, mouthUp33, mouthUp34, mouthUp35, mouthUp36, mouthUp37, mouthUp38, mouthUp39, mouthUp40, mouthUp41, mouthUp42, mouthUp43, mouthUp44, mouthUp45, mouthUp46, mouthUp47, mouthUp48, mouthUp49, mouthUp50, mouthUp51, mouthUp52, mouthUp53, mouthUp54, mouthUp55, mouthUp56, mouthUp57, mouthUp58, mouthUp59, mouthUp60};

    const uint8_t* mouthDown[60] = {mouthDown1, mouthDown2, mouthDown3, mouthDown4, mouthDown5, mouthDown6, mouthDown7, mouthDown8, mouthDown9, mouthDown10, mouthDown11, mouthDown12, mouthDown13, mouthDown14, mouthDown15, mouthDown16, mouthDown17, mouthDown18, mouthDown19, mouthDown20, mouthDown21, mouthDown22, mouthDown23, mouthDown24, mouthDown25, mouthDown26, mouthDown27, mouthDown28, mouthDown29, mouthDown30, mouthDown31, mouthDown32, mouthDown33, mouthDown34, mouthDown35, mouthDown36, mouthDown37, mouthDown38, mouthDown39, mouthDown40, mouthDown41, mouthDown42, mouthDown43, mouthDown44, mouthDown45, mouthDown46, mouthDown47, mouthDown48, mouthDown49, mouthDown50, mouthDown51, mouthDown52, mouthDown53, mouthDown54, mouthDown55, mouthDown56, mouthDown57, mouthDown58, mouthDown59, mouthDown60};

    const uint8_t* mouthAngryAnimation[20] = {mouthAH1, mouthAH2, mouthAH3, mouthAH4, mouthAH5, mouthAH6, mouthAH7, mouthAH8, mouthAH9, mouthAH10, mouthAH11, mouthAH12, mouthAH13, mouthAH14, mouthAH15, mouthAH16, mouthAH17, mouthAH18, mouthAH19, mouthAH20};
    uint8_t angryLength = arrayLength(mouthAngryAnimation);

    const uint8_t* mouthWahAnimation[60] = {mouthWah1, mouthWah2, mouthWah3, mouthWah4, mouthWah5, mouthWah6, mouthWah7, mouthWah8, mouthWah9, mouthWah10, mouthWah11, mouthWah12, mouthWah13, mouthWah14, mouthWah15, mouthWah16, mouthWah17, mouthWah18, mouthWah19, mouthWah20, mouthWah21, mouthWah22, mouthWah23, mouthWah24, mouthWah25, mouthWah26, mouthWah27, mouthWah28, mouthWah29, mouthWah30, mouthWah31, mouthWah32, mouthWah33, mouthWah34, mouthWah35, mouthWah36, mouthWah37, mouthWah38, mouthWah39, mouthWah40, mouthWah41, mouthWah42, mouthWah43, mouthWah44, mouthWah45, mouthWah46, mouthWah47, mouthWah48, mouthWah49, mouthWah50, mouthWah51, mouthWah52, mouthWah53, mouthWah54, mouthWah55, mouthWah56, mouthWah57, mouthWah58, mouthWah59, mouthWah60};
    uint8_t wahLength = arrayLength(mouthWahAnimation);

    const uint8_t* mouthEhAnimation[60] = {mouthEh1, mouthEh2, mouthEh3, mouthEh4, mouthEh5, mouthEh6, mouthEh7, mouthEh8, mouthEh9, mouthEh10, mouthEh11, mouthEh12, mouthEh13, mouthEh14, mouthEh15, mouthEh16, mouthEh17, mouthEh18, mouthEh19, mouthEh20, mouthEh21, mouthEh22, mouthEh23, mouthEh24, mouthEh25, mouthEh26, mouthEh27, mouthEh28, mouthEh29, mouthEh30, mouthEh31, mouthEh32, mouthEh33, mouthEh34, mouthEh35, mouthEh36, mouthEh37, mouthEh38, mouthEh39, mouthEh40, mouthEh41, mouthEh42, mouthEh43, mouthEh44, mouthEh45, mouthEh46, mouthEh47, mouthEh48, mouthEh49, mouthEh50, mouthEh51, mouthEh52, mouthEh53, mouthEh54, mouthEh55, mouthEh56, mouthEh57, mouthEh58, mouthEh59, mouthEh60};
    uint8_t ehLength = arrayLength(mouthEhAnimation);

    const uint8_t* mouthPoutAnimation[60] = {mouthPout1, mouthPout2, mouthPout3, mouthPout4, mouthPout5, mouthPout6, mouthPout7, mouthPout8, mouthPout9, mouthPout10, mouthPout11, mouthPout12, mouthPout13, mouthPout14, mouthPout15, mouthPout16, mouthPout17, mouthPout18, mouthPout19, mouthPout20, mouthPout21, mouthPout22, mouthPout23, mouthPout24, mouthPout25, mouthPout26, mouthPout27, mouthPout28, mouthPout29, mouthPout30, mouthPout31, mouthPout32, mouthPout33, mouthPout34, mouthPout35, mouthPout36, mouthPout37, mouthPout38, mouthPout39, mouthPout40, mouthPout41, mouthPout42, mouthPout43, mouthPout44, mouthPout45, mouthPout46, mouthPout47, mouthPout48, mouthPout49, mouthPout50, mouthPout51, mouthPout52, mouthPout53, mouthPout54, mouthPout55, mouthPout56, mouthPout57, mouthPout58, mouthPout59, mouthPout60};
    uint8_t poutLength = arrayLength(mouthPoutAnimation);

    const uint8_t* mouthDroolingAnimation[60] = {mouthDrooling1, mouthDrooling2, mouthDrooling3, mouthDrooling4, mouthDrooling5, mouthDrooling6, mouthDrooling7, mouthDrooling8, mouthDrooling9, mouthDrooling10, mouthDrooling11, mouthDrooling12, mouthDrooling13, mouthDrooling14, mouthDrooling15, mouthDrooling16, mouthDrooling17, mouthDrooling18, mouthDrooling19, mouthDrooling20, mouthDrooling21, mouthDrooling22, mouthDrooling23, mouthDrooling24, mouthDrooling25, mouthDrooling26, mouthDrooling27, mouthDrooling28, mouthDrooling29, mouthDrooling30, mouthDrooling31, mouthDrooling32, mouthDrooling33, mouthDrooling34, mouthDrooling35, mouthDrooling36, mouthDrooling37, mouthDrooling38, mouthDrooling39, mouthDrooling40, mouthDrooling41, mouthDrooling42, mouthDrooling43, mouthDrooling44, mouthDrooling45, mouthDrooling46, mouthDrooling47, mouthDrooling48, mouthDrooling49, mouthDrooling50, mouthDrooling51, mouthDrooling52, mouthDrooling53, mouthDrooling54, mouthDrooling55, mouthDrooling56, mouthDrooling57, mouthDrooling58, mouthDrooling59, mouthDrooling60};
    uint8_t droolingLength = arrayLength(mouthDroolingAnimation);

    // Animation states - TimeBasedAnimation
    TimeBasedAnimState wahAnim;
    TimeBasedAnimState idleAnim;
    TimeBasedAnimState angryAnim;

    // Animation data structures for transition + loop pattern
    MouthAnimationData ehData;
    MouthAnimationData poutData;
    MouthAnimationData droolingData;

    void drawDefault();
    void movingMouth();
    void angryBoop();
    void playAnimationWithLoop(MouthAnimationData& animData);
    void initAnimationData(MouthAnimationData& data, const uint8_t** frames, uint8_t frameCount,
                           uint8_t loopFrameCount, const TimeBasedAnimConfig& loopConfig);
    void resetAnimation(MouthAnimationData& data);

    TaskHandle_t visemeTaskHandle = NULL;
    void startVisemeTask();
    static void visemeRenderingTask(void* parameter);
};
