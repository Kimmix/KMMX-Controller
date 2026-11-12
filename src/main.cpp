#include <Arduino.h>
#include "KMMXController/KMMXController.h"
#include "Network/BLE.h"

KMMXController controller;
BLEManager& bleManager = BLEManager::getInstance(controller);

void setup() {
    #ifndef DISABLE_SERIAL_LOGGING
    Serial.begin(115200);
    delay(100); // Give serial time to initialize
    #endif

    controller.setupSensors();
    bleManager.setup();
}

void loop() {
    controller.update();
    bleManager.update();
}
