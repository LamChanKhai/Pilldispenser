#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>
#include <LittleFS.h>

#include "audio.h"
#include "config.h"
#include "bp.h"  // Để gọi sendTelegramPillNotTaken()

// ======================= STATE =======================
bool alarmActive = false;
bool playFileActive = false;    // Trạng thái phát file (one-shot)
bool isLooping = false;         // true = alarm (loop), false = play file (one-shot)
File wavFile;
uint32_t wavDataStart = 0;      // Vị trí bắt đầu dữ liệu audio trong file
uint32_t wavDataSize = 0;       // Kích thước dữ liệu audio
uint32_t wavBytesRead = 0;     // Số bytes đã đọc
uint16_t wavSampleRate = 16000; // Sample rate từ WAV file
uint16_t wavBitsPerSample = 16; // Bits per sample từ WAV file
uint16_t wavChannels = 1;       // Số kênh từ WAV file
bool wavFileOpen = false;
static const char* nextWavFilename = nullptr;  // File phát tiếp sau khi one-shot xong (để nối 2 file)

// ======================= ALARM TIMING =======================
unsigned long alarmStartTime = 0;        // Thời gian alarm bắt đầu
bool notificationSent = false;           // Flag đã gửi thông báo chưa uống thuốc
const unsigned long ALARM_TIMEOUT_MS = 300000;  // 5 phút = 300000ms

float audioGain = 3.0f;         // Hệ số tăng âm lượng (3.0 = tăng gấp 3 lần)
int16_t audioBuffer[512];       // Buffer để đọc dữ liệu từ WAV

bool isAlarmActive() { return alarmActive || playFileActive; }

// ======================= WAV FILE PARSING ======================
bool parseWavHeader(File &file) {
    char chunkID[5] = {0};
    uint32_t chunkSize;
    char format[5] = {0};
    
    file.seek(0);
    
    // Đọc RIFF header
    file.readBytes(chunkID, 4);
    if (strncmp(chunkID, "RIFF", 4) != 0) {
        Serial.println("❌ Not a RIFF file");
        return false;
    }
    
    file.readBytes((char*)&chunkSize, 4);
    file.readBytes(format, 4);
    if (strncmp(format, "WAVE", 4) != 0) {
        Serial.println("❌ Not a WAVE file");
        return false;
    }

    // Tìm chunk "fmt "
    bool fmtFound = false;
    while (file.position() < file.size()) {
        file.readBytes(chunkID, 4);
        file.readBytes((char*)&chunkSize, 4);
        
        if (strncmp(chunkID, "fmt ", 4) == 0) {
            fmtFound = true;
            uint16_t audioFormat, numChannels, bitsPerSample, blockAlign;
            uint32_t sampleRate, byteRate;
            
            file.readBytes((char*)&audioFormat, 2);
            file.readBytes((char*)&numChannels, 2);
            file.readBytes((char*)&sampleRate, 4);
            file.readBytes((char*)&byteRate, 4);
            file.readBytes((char*)&blockAlign, 2);
            file.readBytes((char*)&bitsPerSample, 2);
            
            wavSampleRate = sampleRate;
            wavChannels = numChannels;
            wavBitsPerSample = bitsPerSample;
            
            // Bỏ qua phần còn lại của fmt chunk nếu có
            if (chunkSize > 16) {
                file.seek(file.position() + chunkSize - 16);
            }
            break;
        } else {
            // Bỏ qua chunk này
            file.seek(file.position() + chunkSize);
        }
    }

    if (!fmtFound) {
        Serial.println("❌ fmt chunk not found");
        return false;
    }

    // Tìm chunk "data"
    bool dataFound = false;
    while (file.position() < file.size()) {
        file.readBytes(chunkID, 4);
        file.readBytes((char*)&chunkSize, 4);
        
        if (strncmp(chunkID, "data", 4) == 0) {
            dataFound = true;
            wavDataSize = chunkSize;
            wavDataStart = file.position();
            break;
        } else {
            // Bỏ qua chunk này
            file.seek(file.position() + chunkSize);
        }
    }

    if (!dataFound) {
        Serial.println("❌ data chunk not found");
        return false;
    }

    Serial.printf("✅ WAV Info: %dHz, %d-bit, %d channel(s), %d bytes\n", 
                   wavSampleRate, wavBitsPerSample, wavChannels, wavDataSize);
    return true;
}

// ======================= INIT I2S ======================
void initAudioAlarm() {
    // Khởi tạo LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("❌ LittleFS Mount Failed");
        return;
    }
    Serial.println("✅ LittleFS Mounted");

    // Cấu hình I2S với sample rate mặc định (sẽ được cập nhật khi load WAV)
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

    Serial.println("Audio Alarm Ready ✓");
}

// ================ CONTROL FUNCTIONS ===================

// Hàm chung để mở và chuẩn bị phát file WAV
bool openWavFile(const char* filepath, bool loop) {
    // Dừng phát hiện tại nếu có
    if (alarmActive || playFileActive) {
        stopAlarmSound();
    }

    // Mở file WAV
    if (!LittleFS.exists(filepath)) {
        Serial.printf("❌ File %s not found!\n", filepath);
        return false;
    }

    wavFile = LittleFS.open(filepath, "r");
    if (!wavFile) {
        Serial.printf("❌ Cannot open %s\n", filepath);
        return false;
    }

    // Parse WAV header
    if (!parseWavHeader(wavFile)) {
        wavFile.close();
        return false;
    }

    // Cập nhật I2S với sample rate từ WAV file
    i2s_set_sample_rates(I2S_NUM_0, wavSampleRate);

    // Đặt vị trí file đến đầu dữ liệu audio
    wavFile.seek(wavDataStart);
    wavBytesRead = 0;
    wavFileOpen = true;
    isLooping = loop;

    if (loop) {
        alarmActive = true;
        playFileActive = false;
        alarmStartTime = millis();  // Lưu thời gian bắt đầu alarm
        notificationSent = false;    // Reset flag thông báo
        Serial.printf("🔊 Alarm ON - Playing %s (loop)\n", filepath);
    } else {
        alarmActive = false;
        playFileActive = true;
        Serial.printf("🔊 Playing %s (one-shot)\n", filepath);
    }

    return true;
}

void startAlarmSound() {
    openWavFile("/sounds/alarm.wav", true);
}

// Hàm mới: phát file WAV với tên file làm tham số (phát một lần)
void playWavFile(const char* filename) {
    nextWavFilename = nullptr;
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/sounds/%s", filename);
    openWavFile(filepath, false);
}

// Phát file thứ nhất, khi xong tự phát file thứ hai (one-shot cho cả hai)
void playWavFileThen(const char* filename, const char* nextFilename) {
    nextWavFilename = nextFilename;
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/sounds/%s", filename);
    openWavFile(filepath, false);
}

void stopAlarmSound() {
    if (!alarmActive && !playFileActive) return;

    alarmActive = false;
    playFileActive = false;
    wavBytesRead = 0;
    alarmStartTime = 0;      // Reset thời gian alarm
    notificationSent = false; // Reset flag thông báo
    
    if (wavFileOpen && wavFile) {
        wavFile.close();
        wavFileOpen = false;
    }

    i2s_zero_dma_buffer(I2S_NUM_0);
    Serial.println("🔇 Audio OFF");
}

void updateAlarmSound() {
    if ((!alarmActive && !playFileActive) || !wavFileOpen || !wavFile) return;

    // Kiểm tra nếu alarm đang chạy và đã qua 5 phút mà chưa gửi thông báo
    if (alarmActive && alarmStartTime > 0 && !notificationSent) {
        unsigned long elapsed = millis() - alarmStartTime;
        if (elapsed >= ALARM_TIMEOUT_MS) {
            Serial.println("⏰ Alarm đã kêu 5 phút - Gửi thông báo chưa uống thuốc");
            sendTelegramPillNotTaken();
            notificationSent = true;  // Đánh dấu đã gửi để không spam
        }
    }

    // Kiểm tra đã phát hết file chưa
    if (wavBytesRead >= wavDataSize) {
        if (isLooping) {
            // Lặp lại từ đầu (cho alarm)
            wavFile.seek(wavDataStart);
            wavBytesRead = 0;
        } else {
            // One-shot xong: nếu có file tiếp theo thì phát, không thì dừng
            if (nextWavFilename != nullptr) {
                char filepath[64];
                snprintf(filepath, sizeof(filepath), "/sounds/%s", nextWavFilename);
                nextWavFilename = nullptr;
                wavFile.close();
                wavFileOpen = false;
                openWavFile(filepath, false);
                return;
            }
            stopAlarmSound();
            return;
        }
    }

    // Đọc dữ liệu từ WAV file
    size_t bytesToRead = min((uint32_t)sizeof(audioBuffer), wavDataSize - wavBytesRead);
    size_t bytesRead = wavFile.readBytes((char*)audioBuffer, bytesToRead);
    
    if (bytesRead == 0) {
        // File đã hết, reset về đầu
        wavFile.seek(wavDataStart);
        wavBytesRead = 0;
        return;
    }

    // Tăng âm lượng bằng cách nhân với gain và clamp để tránh clipping
    size_t samplesCount = bytesRead / sizeof(int16_t);
    for (size_t i = 0; i < samplesCount; i++) {
        int32_t amplified = (int32_t)audioBuffer[i] * audioGain;
        // Clamp giá trị trong phạm vi int16_t để tránh clipping
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;
        audioBuffer[i] = (int16_t)amplified;
    }

    // Gửi dữ liệu đến I2S
    size_t written;
    i2s_write(I2S_NUM_0, (const char*)audioBuffer, bytesRead, &written, portMAX_DELAY);
    
    wavBytesRead += bytesRead;
}
