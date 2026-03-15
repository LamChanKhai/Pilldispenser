#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#include "audio.h"
#include "config.h"
#include "bp.h"

// ======================= BEEP CONFIG =======================
#define BEEP_FREQ_HZ      1000
#define BEEP_DURATION_MS  150
#define BEEP_PAUSE_MS     200
#define BEEP_AMPLITUDE    12000

static const uint32_t AUDIO_SR = 16000;
static const size_t BUF_SAMPLES = 256;
int16_t audioBuffer[BUF_SAMPLES];

// ======================= STATE =======================
bool alarmActive = false;
bool notificationSent = false;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_TIMEOUT_MS = 30000;  // 5 phút

// Alarm beep state machine
enum AlarmBeepState { ALARM_BEEP_OFF, ALARM_BEEP_ON, ALARM_BEEP_PAUSE };
static AlarmBeepState alarmBeepState = ALARM_BEEP_OFF;
static unsigned long alarmBeepPhaseStart = 0;
static uint32_t alarmBeepSamplesWritten = 0;

bool isAlarmActive() { return alarmActive; }

// Bấm trễ: alarm đã chạy quá 5 phút
bool isAlarmOverdue() {
    if (!alarmActive || alarmStartTime == 0) return false;
    return (millis() - alarmStartTime) >= ALARM_TIMEOUT_MS;
}

// ======================= SINEWAVE BEEP =======================
// Tạo buffer sine wave và ghi ra I2S
static void writeBeepSamples(uint32_t numSamples, bool fadeOut = false) {
    const float omega = 2.0f * 3.14159265f * BEEP_FREQ_HZ / AUDIO_SR;
    uint32_t written = 0;

    while (written < numSamples) {
        size_t chunk = (numSamples - written < BUF_SAMPLES) ? (size_t)(numSamples - written) : BUF_SAMPLES;
        for (size_t i = 0; i < chunk; i++) {
            float t = (written + i) * omega;
            int32_t sample = (int32_t)(BEEP_AMPLITUDE * sinf(t));
            if (fadeOut && written + i > numSamples - AUDIO_SR / 100) {
                float f = (float)(numSamples - (written + i)) / (AUDIO_SR / 100);
                sample = (int32_t)(sample * f);
            }
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            audioBuffer[i] = (int16_t)sample;
        }
        size_t bytesWritten;
        i2s_write(I2S_NUM_0, (const char*)audioBuffer, chunk * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
        written += chunk;
    }
}

// Bíp 1 lần (blocking, ngắn)
void beepOnce() {
    uint32_t samples = AUDIO_SR * BEEP_DURATION_MS / 1000;
    writeBeepSamples(samples, true);
}

// ======================= INIT I2S =======================
void initAudioAlarm() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SR,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = true,
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

    Serial.println("Audio Beep Ready ✓");
}

// ======================= ALARM CONTROL =======================
void startAlarmSound() {
    alarmActive = true;
    alarmStartTime = millis();
    notificationSent = false;
    alarmBeepState = ALARM_BEEP_ON;
    alarmBeepPhaseStart = millis();
    alarmBeepSamplesWritten = 0;
    Serial.println("🔊 Alarm ON - Beep continuous");
}

void stopAlarmSound() {
    alarmActive = false;
    alarmStartTime = 0;
    notificationSent = false;
    alarmBeepState = ALARM_BEEP_OFF;
    i2s_zero_dma_buffer(I2S_NUM_0);
    Serial.println("🔇 Alarm OFF");
}

void updateAlarmSound() {
    if (!alarmActive || alarmBeepState == ALARM_BEEP_OFF) return;

    // Timeout 5 phút → gửi thông báo
    if (alarmStartTime > 0 && !notificationSent) {
        unsigned long elapsed = millis() - alarmStartTime;
        if (elapsed >= ALARM_TIMEOUT_MS) {
            Serial.println("⏰ Alarm 5 phút - Gửi thông báo chưa uống thuốc");
            sendTelegramPillNotTaken();
            notificationSent = true;
        }
    }

    unsigned long now = millis();

    if (alarmBeepState == ALARM_BEEP_ON) {
        uint32_t targetSamples = AUDIO_SR * BEEP_DURATION_MS / 1000;
        if (alarmBeepSamplesWritten >= targetSamples) {
            alarmBeepState = ALARM_BEEP_PAUSE;
            alarmBeepPhaseStart = now;
            return;
        }
        uint32_t remain = targetSamples - alarmBeepSamplesWritten;
        size_t chunk = (remain < BUF_SAMPLES) ? (size_t)remain : BUF_SAMPLES;
        const float omega = 2.0f * 3.14159265f * BEEP_FREQ_HZ / AUDIO_SR;

        for (size_t i = 0; i < chunk; i++) {
            float t = (alarmBeepSamplesWritten + i) * omega;
            int32_t sample = (int32_t)(BEEP_AMPLITUDE * sinf(t));
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            audioBuffer[i] = (int16_t)sample;
        }
        size_t bytesWritten;
        i2s_write(I2S_NUM_0, (const char*)audioBuffer, chunk * sizeof(int16_t), &bytesWritten, 10);
        alarmBeepSamplesWritten += chunk;
    } else if (alarmBeepState == ALARM_BEEP_PAUSE) {
        if (now - alarmBeepPhaseStart >= BEEP_PAUSE_MS) {
            alarmBeepState = ALARM_BEEP_ON;
            alarmBeepPhaseStart = now;
            alarmBeepSamplesWritten = 0;
        }
    }
}
