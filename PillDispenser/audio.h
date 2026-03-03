#ifndef AUDIO_H
#define AUDIO_H

// ================= AUDIO BEEP =================
void initAudioAlarm();     // Khởi tạo I2S
void startAlarmSound();    // Bắt đầu alarm (bíp liên tục đến giờ uống thuốc)
void stopAlarmSound();     // Tắt alarm
void updateAlarmSound();   // Gọi mỗi loop() để duy trì alarm
bool isAlarmActive();      // Đang alarm?

void beepOnce();           // Bíp 1 lần ngắn (MAX3010, BP, v.v.)

#endif
