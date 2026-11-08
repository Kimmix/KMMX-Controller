#include "I2SMicrophone.h"

I2SMicrophone::I2SMicrophone() {}

I2SMicrophone::~I2SMicrophone() {
    stop_i2s();
}

bool I2SMicrophone::i2s_install(uint32_t sampleRate, int samples) {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = samples,
        .use_apll = false};

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver: %s\n", esp_err_to_name(err));
    }
    installed = err == ESP_OK;
    return installed;
}

bool I2SMicrophone::i2s_setpin() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    esp_err_t err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins: %s\n", esp_err_to_name(err));
    }
    return err == ESP_OK;
}

bool I2SMicrophone::init(uint32_t sampleRate, int samples) {
    if (!i2s_install(sampleRate, samples)) return false;
    if (i2s_setpin() && start_i2s()) return true;
    stop_i2s();
    return false;
}

bool I2SMicrophone::read(int16_t* buffer, int samples) {
    if (!installed) return false;
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_PORT, buffer, samples * sizeof(int16_t), &bytesRead, pdMS_TO_TICKS(100));
    return err == ESP_OK && bytesRead == samples * sizeof(int16_t);
}

bool I2SMicrophone::start_i2s() {
    return installed && i2s_start(I2S_PORT) == ESP_OK;
}

void I2SMicrophone::stop_i2s() {
    if (!installed) return;
    i2s_stop(I2S_PORT);
    i2s_driver_uninstall(I2S_PORT);
    installed = false;
}
