#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

class I2SMicrophone {
   private:
    bool installed = false;
    bool i2s_install(uint32_t sampleRate, int samples);
    bool i2s_setpin();

   public:
    I2SMicrophone();
    ~I2SMicrophone();
    bool init(uint32_t sampleRate, int samples);
    bool read(int32_t* buffer, int samples);
    bool start_i2s();
    void stop_i2s();
};