#ifndef SCHEDULE_H
#define SCHEDULE_H

// ======================= LỊCH UỐNG THUỐC =========================
// Chứa tối đa 12 lần nhắc trong ngày
// Mỗi mục lưu dạng HH:MM

void clearSchedule();                         // Xóa toàn bộ lịch
void setSchedule(String data);                // Nhận lịch từ MQTT/json/csv
void checkSchedule(void (*dispenseFunc)());   // So khớp thời gian → gọi hàm cấp thuốc

#endif
