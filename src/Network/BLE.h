#pragma once
#include <NimBLEDevice.h>
#include "KMMXController/KMMXController.h"

class BLEManager {
   public:
    static BLEManager& getInstance(KMMXController& controller);
    void setup();
    void update();
    bool isConnected() const;

    static BLEManager* instance;  // Made public for callbacks to access
    KMMXController& controller;

#ifdef DISABLE_SERIAL_LOGGING
    bool debugEnabled = false;  // Disabled in production builds
#else
    bool debugEnabled = true;   // Flag to control debug output
#endif

   private:
    BLEManager(KMMXController& controller);
    NimBLEServer* pServer;
    NimBLEService* pService;
    NimBLECharacteristic* visemeStatus = nullptr;
    NimBLECharacteristic* visemeScores = nullptr;
    uint32_t lastScoresNotify = 0;
    uint8_t lastVisemeStatus[LipSync::StatusSize] = {};
    uint32_t lastVisemeNotify = 0;
    NimBLECharacteristic* orientationStatus = nullptr;
    uint32_t lastOrientationNotify = 0;
};
