#pragma once
#include <atomic>
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
                            DROOLING,
                            ANGRY,
                            LOWER,
                            SHOCK,
                            SMALL,
                            WORRY };

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
    void setBoopCompleted(bool completed);
    void releaseBoop(unsigned long durationMs);
    void resetMovingMouth();
    void setSlowAnimation(bool slow);

   private:
    Hub75DMA* display;
    SensorData sensorData;
    bool boopCompleted = false;
    MouthStateEnum prevState = MouthStateEnum::IDLE;
    std::atomic<MouthStateEnum> currentState{MouthStateEnum::IDLE};
    unsigned long mouthInterval, nextAngry;
    unsigned long stateStartTime = 0;       // When current state started (for auto-reset)
    unsigned long customResetDuration = 0;  // Custom duration override (0 = infinite)
    bool isTransitioning = false;
    bool isReturning = false;
    MouthStateEnum pendingState = MouthStateEnum::IDLE;
    bool pendingPersistent = false;
    unsigned long pendingDuration = 0;
    TimeBasedAnimState returnAnim;
    std::atomic<const uint8_t*> visemeFrame{mouthDefault};
    const uint8_t* mouthFrame = mouthDefault;

    const uint8_t* defaultAnimation[60] = {mouthDefault1, mouthDefault2, mouthDefault3, mouthDefault4, mouthDefault5, mouthDefault6, mouthDefault7, mouthDefault8, mouthDefault9, mouthDefault10, mouthDefault11, mouthDefault12, mouthDefault13, mouthDefault14, mouthDefault15, mouthDefault16, mouthDefault17, mouthDefault18, mouthDefault19, mouthDefault20, mouthDefault21, mouthDefault22, mouthDefault23, mouthDefault24, mouthDefault25, mouthDefault26, mouthDefault27, mouthDefault28, mouthDefault29, mouthDefault30, mouthDefault31, mouthDefault32, mouthDefault33, mouthDefault34, mouthDefault35, mouthDefault36, mouthDefault37, mouthDefault38, mouthDefault39, mouthDefault40, mouthDefault41, mouthDefault42, mouthDefault43, mouthDefault44, mouthDefault45, mouthDefault46, mouthDefault47, mouthDefault48, mouthDefault49, mouthDefault50, mouthDefault51, mouthDefault52, mouthDefault53, mouthDefault54, mouthDefault55, mouthDefault56, mouthDefault57, mouthDefault58, mouthDefault59, mouthDefault60};
    uint8_t defaultAnimationLength = arrayLength(defaultAnimation);

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

    const uint8_t* mouthAngryNewAnimation[60] = {mouthAngry1, mouthAngry2, mouthAngry3, mouthAngry4, mouthAngry5, mouthAngry6, mouthAngry7, mouthAngry8, mouthAngry9, mouthAngry10, mouthAngry11, mouthAngry12, mouthAngry13, mouthAngry14, mouthAngry15, mouthAngry16, mouthAngry17, mouthAngry18, mouthAngry19, mouthAngry20, mouthAngry21, mouthAngry22, mouthAngry23, mouthAngry24, mouthAngry25, mouthAngry26, mouthAngry27, mouthAngry28, mouthAngry29, mouthAngry30, mouthAngry31, mouthAngry32, mouthAngry33, mouthAngry34, mouthAngry35, mouthAngry36, mouthAngry37, mouthAngry38, mouthAngry39, mouthAngry40, mouthAngry41, mouthAngry42, mouthAngry43, mouthAngry44, mouthAngry45, mouthAngry46, mouthAngry47, mouthAngry48, mouthAngry49, mouthAngry50, mouthAngry51, mouthAngry52, mouthAngry53, mouthAngry54, mouthAngry55, mouthAngry56, mouthAngry57, mouthAngry58, mouthAngry59, mouthAngry60};
    uint8_t angryNewLength = arrayLength(mouthAngryNewAnimation);

    const uint8_t* mouthLowerAnimation[60] = {mouthLower1, mouthLower2, mouthLower3, mouthLower4, mouthLower5, mouthLower6, mouthLower7, mouthLower8, mouthLower9, mouthLower10, mouthLower11, mouthLower12, mouthLower13, mouthLower14, mouthLower15, mouthLower16, mouthLower17, mouthLower18, mouthLower19, mouthLower20, mouthLower21, mouthLower22, mouthLower23, mouthLower24, mouthLower25, mouthLower26, mouthLower27, mouthLower28, mouthLower29, mouthLower30, mouthLower31, mouthLower32, mouthLower33, mouthLower34, mouthLower35, mouthLower36, mouthLower37, mouthLower38, mouthLower39, mouthLower40, mouthLower41, mouthLower42, mouthLower43, mouthLower44, mouthLower45, mouthLower46, mouthLower47, mouthLower48, mouthLower49, mouthLower50, mouthLower51, mouthLower52, mouthLower53, mouthLower54, mouthLower55, mouthLower56, mouthLower57, mouthLower58, mouthLower59, mouthLower60};
    uint8_t lowerLength = arrayLength(mouthLowerAnimation);

    const uint8_t* mouthShockAnimation[60] = {mouthShock1, mouthShock2, mouthShock3, mouthShock4, mouthShock5, mouthShock6, mouthShock7, mouthShock8, mouthShock9, mouthShock10, mouthShock11, mouthShock12, mouthShock13, mouthShock14, mouthShock15, mouthShock16, mouthShock17, mouthShock18, mouthShock19, mouthShock20, mouthShock21, mouthShock22, mouthShock23, mouthShock24, mouthShock25, mouthShock26, mouthShock27, mouthShock28, mouthShock29, mouthShock30, mouthShock31, mouthShock32, mouthShock33, mouthShock34, mouthShock35, mouthShock36, mouthShock37, mouthShock38, mouthShock39, mouthShock40, mouthShock41, mouthShock42, mouthShock43, mouthShock44, mouthShock45, mouthShock46, mouthShock47, mouthShock48, mouthShock49, mouthShock50, mouthShock51, mouthShock52, mouthShock53, mouthShock54, mouthShock55, mouthShock56, mouthShock57, mouthShock58, mouthShock59, mouthShock60};
    uint8_t shockLength = arrayLength(mouthShockAnimation);

    const uint8_t* mouthSmallAnimation[60] = {mouthSmall1, mouthSmall2, mouthSmall3, mouthSmall4, mouthSmall5, mouthSmall6, mouthSmall7, mouthSmall8, mouthSmall9, mouthSmall10, mouthSmall11, mouthSmall12, mouthSmall13, mouthSmall14, mouthSmall15, mouthSmall16, mouthSmall17, mouthSmall18, mouthSmall19, mouthSmall20, mouthSmall21, mouthSmall22, mouthSmall23, mouthSmall24, mouthSmall25, mouthSmall26, mouthSmall27, mouthSmall28, mouthSmall29, mouthSmall30, mouthSmall31, mouthSmall32, mouthSmall33, mouthSmall34, mouthSmall35, mouthSmall36, mouthSmall37, mouthSmall38, mouthSmall39, mouthSmall40, mouthSmall41, mouthSmall42, mouthSmall43, mouthSmall44, mouthSmall45, mouthSmall46, mouthSmall47, mouthSmall48, mouthSmall49, mouthSmall50, mouthSmall51, mouthSmall52, mouthSmall53, mouthSmall54, mouthSmall55, mouthSmall56, mouthSmall57, mouthSmall58, mouthSmall59, mouthSmall60};
    uint8_t smallLength = arrayLength(mouthSmallAnimation);

    const uint8_t* mouthWorryAnimation[60] = {mouthWorry1, mouthWorry2, mouthWorry3, mouthWorry4, mouthWorry5, mouthWorry6, mouthWorry7, mouthWorry8, mouthWorry9, mouthWorry10, mouthWorry11, mouthWorry12, mouthWorry13, mouthWorry14, mouthWorry15, mouthWorry16, mouthWorry17, mouthWorry18, mouthWorry19, mouthWorry20, mouthWorry21, mouthWorry22, mouthWorry23, mouthWorry24, mouthWorry25, mouthWorry26, mouthWorry27, mouthWorry28, mouthWorry29, mouthWorry30, mouthWorry31, mouthWorry32, mouthWorry33, mouthWorry34, mouthWorry35, mouthWorry36, mouthWorry37, mouthWorry38, mouthWorry39, mouthWorry40, mouthWorry41, mouthWorry42, mouthWorry43, mouthWorry44, mouthWorry45, mouthWorry46, mouthWorry47, mouthWorry48, mouthWorry49, mouthWorry50, mouthWorry51, mouthWorry52, mouthWorry53, mouthWorry54, mouthWorry55, mouthWorry56, mouthWorry57, mouthWorry58, mouthWorry59, mouthWorry60};
    uint8_t worryLength = arrayLength(mouthWorryAnimation);

    // Animation states - TimeBasedAnimation
    TimeBasedAnimState wahAnim;
    TimeBasedAnimState idleAnim;
    TimeBasedAnimState angryAnim;
    bool slowIdleAnimation = false;
    bool idleSigh = false;
    uint8_t breathsUntilSigh = 10;

    // Animation data structures for transition + loop pattern
    MouthAnimationData boopData;
    MouthAnimationData ehData;
    MouthAnimationData poutData;
    MouthAnimationData droolingData;
    MouthAnimationData angryNewData;
    MouthAnimationData lowerData;
    MouthAnimationData shockData;
    MouthAnimationData smallData;
    MouthAnimationData worryData;

    void drawDefault();
    void movingMouth();
    void angryBoop();
    void playAnimationWithLoop(MouthAnimationData& animData);
    void initAnimationData(MouthAnimationData& data, const uint8_t** frames, uint8_t frameCount,
                           uint8_t loopFrameCount, const TimeBasedAnimConfig& loopConfig);
    void resetAnimation(MouthAnimationData& data);
    void applyState(MouthStateEnum newState, bool isPersistent, unsigned long durationMs);
    bool getAnimationFrames(MouthStateEnum state, const uint8_t**& frames, uint8_t& frameCount);

    TaskHandle_t visemeTaskHandle = NULL;
    void startVisemeTask();
    static void visemeRenderingTask(void* parameter);
};
