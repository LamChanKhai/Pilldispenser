#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#include "audio.h"
#include "config.h"

// ======================= STATE =======================
bool alarmActive = false;
int16_t audioBuffer[256];   // xung sin tạo âm thông báo

bool isAlarmActive() { return alarmActive; }

// ======================= INIT I2S ======================
void initAudioAlarm() {

    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pins = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num  = I2S_LRC,
        .data_out_num = I2S_DATA,
        .data_in_num = -1
    };

    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);
    i2s_zero_dma_buffer(I2S_NUM_0);

    // 🔊 Generate sine wave for beeping tone
    for (int i = 0; i < 256; i++) {
        float p = 2.0f * PI * AUDIO_FREQ * i / SAMPLE_RATE;
        audioBuffer[i] = sin(p) * 28000;
    }

    Serial.println("Audio Alarm Ready ✓");
}

// ================ CONTROL FUNCTIONS ===================

void startAlarmSound() {
    if (!alarmActive) {
        alarmActive = true;
        Serial.println("🔊 Alarm ON");
    }
}

void stopAlarmSound() {
    if (alarmActive) {
        alarmActive = false;
        i2s_zero_dma_buffer(I2S_NUM_0);
        Serial.println("🔇 Alarm OFF");
    }
}

void updateAlarmSound() {
    if (!alarmActive) return;

    size_t written;
    i2s_write(I2S_NUM_0, (const char*)audioBuffer, sizeof(audioBuffer), &written, portMAX_DELAY);
}
