#ifndef AUDIO_H
#define AUDIO_H

// ================= AUDIO BUZZER I2S =================
void initAudioAlarm();     // Khởi tạo I2S + tạo sóng âm
void startAlarmSound();    // Bắt đầu cảnh báo
void stopAlarmSound();     // Tắt cảnh báo
void updateAlarmSound();   // Duy trì âm mỗi chu kỳ loop()
bool isAlarmActive();      // Kiểm tra có đang kêu hay không

#endif
