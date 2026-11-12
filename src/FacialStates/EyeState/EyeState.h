#pragma once

#include <Adafruit_Sensor.h>
#include "Devices/LEDMatrixDisplay/Hub75DMA.h"
#include "Bitmaps/Bitmaps.h"
#include "Utils/Utils.h"
#include "Renderer/GooglyEye.h"
#include "Renderer/TimeBasedAnimation.h"
#include "Types/SensorData.h"

enum class EyeStateEnum { IDLE,
                          BLINK,
                          BOOP,
                          ARROW,
                          GOOGLY,
                          OEYE,
                          HEART,
                          SMILE,
                          ANGRY,
                          SLEEP,
                          SAD,
                          CRY,
                          DOUBTED,
                          ROUNDED,
                          SHARP,
                          GIGGLE,
                          UNIMPRESSED,
                          DETRANSITION };

// Unified animation data structure to reduce repetition
struct AnimationData {
    const uint8_t** frames;           // Array of frame pointers
    uint8_t frameCount;               // Total number of frames
    TimeBasedAnimState transitionAnim; // Full animation (plays once during transition)
    TimeBasedAnimState loopAnim;      // Loop animation (subset of frames, loops continuously)
    uint8_t loopFrameCount;           // Number of frames to use for looping
    unsigned long autoResetDuration;  // Auto-reset to previous state after this duration (0 = no auto-reset)
};

class EyeState {
   public:
    EyeState(Hub75DMA* display);
    void update();
    void setState(EyeStateEnum newState, bool isPersistent = false, unsigned long durationMs = 0);
    void savePrevState(EyeStateEnum newState);
    void playPrevState();
    EyeStateEnum getState() const;
    void setSensorData(const SensorData& data);
    void setBoopCompleted(bool completed);
    void releaseBoop(unsigned long durationMs);

   private:
    Hub75DMA* display;
    SensorData sensorData;
    bool boopCompleted = false;
    GooglyEye googlyEye;
    EyeStateEnum prevState = EyeStateEnum::IDLE, currentState = EyeStateEnum::IDLE;
    bool isTransitioning = false;
    bool isReturning = false;
    EyeStateEnum pendingState = EyeStateEnum::IDLE;
    bool pendingPersistent = false;
    unsigned long pendingDuration = 0;
    TimeBasedAnimState returnAnim;

    unsigned long stateStartTime;  // When current state started (for auto-reset)
    unsigned long customResetDuration = 0;  // Custom duration override (0 = use animation's autoResetDuration)
    unsigned long nextBlink;
    unsigned long nextIdleAction;  // Timer for micro-movements and eye darts

    // Idle micro-movement frames for variety
    const uint8_t* idleLookFrames[10] = {eyeDefault, eyeUp5, eyeUp10, eyeLookSharp5, eyeLookSharp10, eyeGiggle14, eyeUnimpressed14, eyeUnimpressed20, eyeSad8, eyeSmile10};
    const uint8_t* idleTransitionFrames[10][3] = {
        {eyeDefault, eyeDefault, eyeDefault},
        {eyeUp1, eyeUp3, eyeUp4},
        {eyeUp2, eyeUp5, eyeUp8},
        {eyeLookSharp1, eyeLookSharp3, eyeLookSharp4},
        {eyeLookSharp2, eyeLookSharp5, eyeLookSharp8},
        {eyeGiggle3, eyeGiggle7, eyeGiggle10},
        {eyeUnimpressed3, eyeUnimpressed7, eyeUnimpressed10},
        {eyeUnimpressed5, eyeUnimpressed10, eyeUnimpressed15},
        {eyeSad2, eyeSad4, eyeSad6},
        {eyeSmile2, eyeSmile5, eyeSmile8}};
    const uint8_t* currentIdleBitmap = eyeDefault;
    uint8_t currentIdleFrame = 0;
    uint8_t pendingIdleFrame = 0;
    uint8_t idleTransitionStep = 0;
    bool idleCorrectionMade = false;
    const uint8_t idleCorrectionFrames[10] = {0, 2, 1, 4, 3, 0, 7, 6, 0, 0};

    // Accelerometer-based eye movement animations (currently disabled)
    const uint8_t* eyeDownAnimation[20] = {
        eyeDown1, eyeDown2, eyeDown3, eyeDown4, eyeDown5, eyeDown6, eyeDown7, eyeDown8, eyeDown9, eyeDown10,
        eyeDown11, eyeDown12, eyeDown13, eyeDown14, eyeDown15, eyeDown16, eyeDown17, eyeDown18, eyeDown19, eyeDown20};
    const uint8_t* eyeUpAnimation[20] = {
        eyeUp1, eyeUp2, eyeUp3, eyeUp4, eyeUp5, eyeUp6, eyeUp7, eyeUp8, eyeUp9, eyeUp10,
        eyeUp11, eyeUp12, eyeUp13, eyeUp14, eyeUp15, eyeUp16, eyeUp17, eyeUp18, eyeUp19, eyeUp20};

    // Blink animation (special case - no loop, just transition)
    const uint8_t* blinkAnimation[24] = {eyeBlink1, eyeBlink2, eyeBlink3, eyeBlink4, eyeBlink5, eyeBlink6, eyeBlink7, eyeBlink8, eyeBlink9, eyeBlink10, eyeBlink11, eyeBlink12, eyeBlink13, eyeBlink14, eyeBlink15, eyeBlink16, eyeBlink17, eyeBlink18, eyeBlink19, eyeBlink20, eyeBlink21, eyeBlink22, eyeBlink23, eyeBlink24};
    const uint8_t blinkAnimationLength = arrayLength(blinkAnimation);
    TimeBasedAnimState blinkAnim;
    unsigned long blinkResumeAt = 0;  // non-zero while paused between double-blink halves
    uint8_t blinkCount = 0;
    bool shouldDoubleBlink = false;

    // Unified animation storage for states with transition + loop pattern
    const uint8_t* boopAnimation[48] = {
        eyeArrow1, eyeArrow2, eyeArrow3, eyeArrow4, eyeArrow5, eyeArrow6, eyeArrow7, eyeArrow8, eyeArrow9, eyeArrow10,
        eyeArrow11, eyeArrow12, eyeArrow13, eyeArrow14, eyeArrow15, eyeArrow16, eyeArrow17, eyeArrow18, eyeArrow19, eyeArrow20,
        eyeArrow21, eyeArrow22, eyeArrow23, eyeArrow24, eyeArrow25, eyeArrow26, eyeArrow27, eyeArrow28, eyeArrow29, eyeArrow30,
        eyeArrow31, eyeArrow32, eyeArrow33, eyeArrow34, eyeArrow35, eyeArrow36, eyeArrow37, eyeArrow38, eyeArrow39, eyeArrow40,
        eyeArrow41, eyeArrow42, eyeArrow43, eyeArrow44, eyeArrow45, eyeArrow46, eyeArrow47, eyeArrow48};

    const uint8_t* oFaceAnimation[48] = {
        eyeCircle1, eyeCircle2, eyeCircle3, eyeCircle4, eyeCircle5, eyeCircle6, eyeCircle7, eyeCircle8, eyeCircle9, eyeCircle10,
        eyeCircle11, eyeCircle12, eyeCircle13, eyeCircle14, eyeCircle15, eyeCircle16, eyeCircle17, eyeCircle18, eyeCircle19, eyeCircle20,
        eyeCircle21, eyeCircle22, eyeCircle23, eyeCircle24, eyeCircle25, eyeCircle26, eyeCircle27, eyeCircle28, eyeCircle29, eyeCircle30,
        eyeCircle31, eyeCircle32, eyeCircle33, eyeCircle34, eyeCircle35, eyeCircle36, eyeCircle37, eyeCircle38, eyeCircle39, eyeCircle40,
        eyeCircle41, eyeCircle42, eyeCircle43, eyeCircle44, eyeCircle45, eyeCircle46, eyeCircle47, eyeCircle48};

    const uint8_t* smileAnimation[48] = {
        eyeSmile1, eyeSmile2, eyeSmile3, eyeSmile4, eyeSmile5, eyeSmile6, eyeSmile7, eyeSmile8, eyeSmile9, eyeSmile10,
        eyeSmile11, eyeSmile12, eyeSmile13, eyeSmile14, eyeSmile15, eyeSmile16, eyeSmile17, eyeSmile18, eyeSmile19, eyeSmile20,
        eyeSmile21, eyeSmile22, eyeSmile23, eyeSmile24, eyeSmile25, eyeSmile26, eyeSmile27, eyeSmile28, eyeSmile29, eyeSmile30,
        eyeSmile31, eyeSmile32, eyeSmile33, eyeSmile34, eyeSmile35, eyeSmile36, eyeSmile37, eyeSmile38, eyeSmile39, eyeSmile40,
        eyeSmile41, eyeSmile42, eyeSmile43, eyeSmile44, eyeSmile45, eyeSmile46, eyeSmile47, eyeSmile48};

    const uint8_t* eyeAngryAnimation[48] = {
        eyeAngry1, eyeAngry2, eyeAngry3, eyeAngry4, eyeAngry5, eyeAngry6, eyeAngry7, eyeAngry8, eyeAngry9, eyeAngry10,
        eyeAngry11, eyeAngry12, eyeAngry13, eyeAngry14, eyeAngry15, eyeAngry16, eyeAngry17, eyeAngry18, eyeAngry19, eyeAngry20,
        eyeAngry21, eyeAngry22, eyeAngry23, eyeAngry24, eyeAngry25, eyeAngry26, eyeAngry27, eyeAngry28, eyeAngry29, eyeAngry30,
        eyeAngry31, eyeAngry32, eyeAngry33, eyeAngry34, eyeAngry35, eyeAngry36, eyeAngry37, eyeAngry38, eyeAngry39, eyeAngry40,
        eyeAngry41, eyeAngry42, eyeAngry43, eyeAngry44, eyeAngry45, eyeAngry46, eyeAngry47, eyeAngry48};

    const uint8_t* eyeSleepAnimation[48] = {
        eyeSleep1, eyeSleep2, eyeSleep3, eyeSleep4, eyeSleep5, eyeSleep6, eyeSleep7, eyeSleep8, eyeSleep9, eyeSleep10,
        eyeSleep11, eyeSleep12, eyeSleep13, eyeSleep14, eyeSleep15, eyeSleep16, eyeSleep17, eyeSleep18, eyeSleep19, eyeSleep20,
        eyeSleep21, eyeSleep22, eyeSleep23, eyeSleep24, eyeSleep25, eyeSleep26, eyeSleep27, eyeSleep28, eyeSleep29, eyeSleep30,
        eyeSleep31, eyeSleep32, eyeSleep33, eyeSleep34, eyeSleep35, eyeSleep36, eyeSleep37, eyeSleep38, eyeSleep39, eyeSleep40,
        eyeSleep41, eyeSleep42, eyeSleep43, eyeSleep44, eyeSleep45, eyeSleep46, eyeSleep47, eyeSleep48};
    const uint8_t sleepLength = arrayLength(eyeSleepAnimation);

    const uint8_t* sadAnimation[20] = {
        eyeSad1, eyeSad2, eyeSad3, eyeSad4, eyeSad5, eyeSad6, eyeSad7, eyeSad8, eyeSad9, eyeSad10,
        eyeSad11, eyeSad12, eyeSad13, eyeSad14, eyeSad15, eyeSad16, eyeSad17, eyeSad18, eyeSad19, eyeSad20};

    const uint8_t* cryAnimation[48] = {
        eyeCry1, eyeCry2, eyeCry3, eyeCry4, eyeCry5, eyeCry6, eyeCry7, eyeCry8, eyeCry9, eyeCry10,
        eyeCry11, eyeCry12, eyeCry13, eyeCry14, eyeCry15, eyeCry16, eyeCry17, eyeCry18, eyeCry19, eyeCry20,
        eyeCry21, eyeCry22, eyeCry23, eyeCry24, eyeCry25, eyeCry26, eyeCry27, eyeCry28, eyeCry29, eyeCry30,
        eyeCry31, eyeCry32, eyeCry33, eyeCry34, eyeCry35, eyeCry36, eyeCry37, eyeCry38, eyeCry39, eyeCry40,
        eyeCry41, eyeCry42, eyeCry43, eyeCry44, eyeCry45, eyeCry46, eyeCry47, eyeCry48};

    const uint8_t* doubtedAnimation[48] = {
        eyeDoubted1, eyeDoubted2, eyeDoubted3, eyeDoubted4, eyeDoubted5, eyeDoubted6, eyeDoubted7, eyeDoubted8, eyeDoubted9, eyeDoubted10,
        eyeDoubted11, eyeDoubted12, eyeDoubted13, eyeDoubted14, eyeDoubted15, eyeDoubted16, eyeDoubted17, eyeDoubted18, eyeDoubted19, eyeDoubted20,
        eyeDoubted21, eyeDoubted22, eyeDoubted23, eyeDoubted24, eyeDoubted25, eyeDoubted26, eyeDoubted27, eyeDoubted28, eyeDoubted29, eyeDoubted30,
        eyeDoubted31, eyeDoubted32, eyeDoubted33, eyeDoubted34, eyeDoubted35, eyeDoubted36, eyeDoubted37, eyeDoubted38, eyeDoubted39, eyeDoubted40,
        eyeDoubted41, eyeDoubted42, eyeDoubted43, eyeDoubted44, eyeDoubted45, eyeDoubted46, eyeDoubted47, eyeDoubted48};

    const uint8_t* roundedAnimation[48] = {
        eyeRounded1, eyeRounded2, eyeRounded3, eyeRounded4, eyeRounded5, eyeRounded6, eyeRounded7, eyeRounded8, eyeRounded9, eyeRounded10,
        eyeRounded11, eyeRounded12, eyeRounded13, eyeRounded14, eyeRounded15, eyeRounded16, eyeRounded17, eyeRounded18, eyeRounded19, eyeRounded20,
        eyeRounded21, eyeRounded22, eyeRounded23, eyeRounded24, eyeRounded25, eyeRounded26, eyeRounded27, eyeRounded28, eyeRounded29, eyeRounded30,
        eyeRounded31, eyeRounded32, eyeRounded33, eyeRounded34, eyeRounded35, eyeRounded36, eyeRounded37, eyeRounded38, eyeRounded39, eyeRounded40,
        eyeRounded41, eyeRounded42, eyeRounded43, eyeRounded44, eyeRounded45, eyeRounded46, eyeRounded47, eyeRounded48};

    const uint8_t* sharpAnimation[48] = {
        eyeSharp1, eyeSharp2, eyeSharp3, eyeSharp4, eyeSharp5, eyeSharp6, eyeSharp7, eyeSharp8, eyeSharp9, eyeSharp10,
        eyeSharp11, eyeSharp12, eyeSharp13, eyeSharp14, eyeSharp15, eyeSharp16, eyeSharp17, eyeSharp18, eyeSharp19, eyeSharp20,
        eyeSharp21, eyeSharp22, eyeSharp23, eyeSharp24, eyeSharp25, eyeSharp26, eyeSharp27, eyeSharp28, eyeSharp29, eyeSharp30,
        eyeSharp31, eyeSharp32, eyeSharp33, eyeSharp34, eyeSharp35, eyeSharp36, eyeSharp37, eyeSharp38, eyeSharp39, eyeSharp40,
        eyeSharp41, eyeSharp42, eyeSharp43, eyeSharp44, eyeSharp45, eyeSharp46, eyeSharp47, eyeSharp48};

    const uint8_t* giggleAnimation[48] = {
        eyeGiggle1, eyeGiggle2, eyeGiggle3, eyeGiggle4, eyeGiggle5, eyeGiggle6, eyeGiggle7, eyeGiggle8, eyeGiggle9, eyeGiggle10,
        eyeGiggle11, eyeGiggle12, eyeGiggle13, eyeGiggle14, eyeGiggle15, eyeGiggle16, eyeGiggle17, eyeGiggle18, eyeGiggle19, eyeGiggle20,
        eyeGiggle21, eyeGiggle22, eyeGiggle23, eyeGiggle24, eyeGiggle25, eyeGiggle26, eyeGiggle27, eyeGiggle28, eyeGiggle29, eyeGiggle30,
        eyeGiggle31, eyeGiggle32, eyeGiggle33, eyeGiggle34, eyeGiggle35, eyeGiggle36, eyeGiggle37, eyeGiggle38, eyeGiggle39, eyeGiggle40,
        eyeGiggle41, eyeGiggle42, eyeGiggle43, eyeGiggle44, eyeGiggle45, eyeGiggle46, eyeGiggle47, eyeGiggle48};

    const uint8_t* unimpressedAnimation[48] = {
        eyeUnimpressed1, eyeUnimpressed2, eyeUnimpressed3, eyeUnimpressed4, eyeUnimpressed5, eyeUnimpressed6, eyeUnimpressed7, eyeUnimpressed8, eyeUnimpressed9, eyeUnimpressed10,
        eyeUnimpressed11, eyeUnimpressed12, eyeUnimpressed13, eyeUnimpressed14, eyeUnimpressed15, eyeUnimpressed16, eyeUnimpressed17, eyeUnimpressed18, eyeUnimpressed19, eyeUnimpressed20,
        eyeUnimpressed21, eyeUnimpressed22, eyeUnimpressed23, eyeUnimpressed24, eyeUnimpressed25, eyeUnimpressed26, eyeUnimpressed27, eyeUnimpressed28, eyeUnimpressed29, eyeUnimpressed30,
        eyeUnimpressed31, eyeUnimpressed32, eyeUnimpressed33, eyeUnimpressed34, eyeUnimpressed35, eyeUnimpressed36, eyeUnimpressed37, eyeUnimpressed38, eyeUnimpressed39, eyeUnimpressed40,
        eyeUnimpressed41, eyeUnimpressed42, eyeUnimpressed43, eyeUnimpressed44, eyeUnimpressed45, eyeUnimpressed46, eyeUnimpressed47, eyeUnimpressed48};

    // Animation data structures (initialized in constructor)
    AnimationData boopData;
    AnimationData arrowData;
    AnimationData oFaceData;
    AnimationData smileData;
    AnimationData angryData;
    AnimationData sadData;
    AnimationData cryData;
    AnimationData doubtedData;
    AnimationData roundedData;
    AnimationData sharpData;
    AnimationData giggleData;
    AnimationData unimpressedData;

    // Sleep animation (special case - gradual eye closing)
    uint8_t sleepIndex = 0;
    unsigned long startSleepTime;
    unsigned long nextSleepFrameChange;

    // Helper methods
    void idleFace();
    void updateIdleMicroMovements();
    void checkAndTriggerBlink();
    unsigned long selectBlinkInterval();
    const TimeBasedAnimConfig* selectBlinkSpeed(bool isDoubleBlink);
    void blink();
    void playAnimationWithLoop(AnimationData& animData);  // Generic animation playback
    void sleepFace();
    void resetSleepFace();
    void detransition();
    void renderGooglyEye();
    void initAnimationData(AnimationData& data, const uint8_t** frames, uint8_t frameCount,
                          uint8_t loopFrameCount, unsigned long autoResetDuration,
                          const TimeBasedAnimConfig& loopConfig);
    void resetAnimation(AnimationData& data);
    AnimationData* getAnimationData(EyeStateEnum state);
    void applyState(EyeStateEnum newState, bool isPersistent, unsigned long durationMs);
};
