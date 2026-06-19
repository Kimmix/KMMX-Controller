#include <Arduino.h>
#include "config.h"
#include "KMMXController/KMMXController.h"
#include "Network/BLE.h"
#include "Utils/Utils.h"

KMMXController controller;
BLEManager& bleManager = BLEManager::getInstance(controller);

void printBootInfo() {
    #ifndef DISABLE_SERIAL_LOGGING
    Serial.print(F("Booting "));
    Serial.println(F(BOARD_NAME));
    #endif
}

void setup() {
    #ifndef DISABLE_SERIAL_LOGGING
    Serial.begin(115200);
    delay(100); // Give serial time to initialize
    #endif

    printBootInfo();

    controller.setupSensors();
    bleManager.setup();
}

void loop() {
    controller.update();
    bleManager.update();
}
