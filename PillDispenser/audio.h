#ifndef AUDIO_H
#define AUDIO_H

// ================= AUDIO BUZZER I2S =================
void initAudioAlarm();     // Khởi tạo I2S + tạo sóng âm
void startAlarmSound();    // Bắt đầu cảnh báo (loop)
void stopAlarmSound();     // Tắt cảnh báo
void updateAlarmSound();   // Duy trì âm mỗi chu kỳ loop()
bool isAlarmActive();      // Kiểm tra có đang kêu hay không
void playWavFile(const char* filename);  // Phát file WAV một lần (filename trong thư mục /sounds/)
void playWavFileThen(const char* filename, const char* nextFilename);  // Phát file rồi phát file tiếp theo khi xong

#endif
