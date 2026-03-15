#ifndef SCHEDULE_H
#define SCHEDULE_H

// ======================= LỊCH UỐNG THUỐC =========================
// Chứa tối đa 12 lần nhắc trong ngày
// Mỗi mục lưu dạng HH:MM

void clearSchedule();                         // Xóa toàn bộ lịch
void setSchedule(String data);                // Nhận lịch từ MQTT/json/csv
void checkSchedule(void (*dispenseFunc)());   // So khớp thời gian → gọi hàm cấp thuốc
int getLastTriggeredIndex();                  // Chỉ số ô vừa kích hoạt (bấm đúng giờ)
int getTriggeredCount();                      // Số ô đã kích hoạt từ lần bấm trước
int getTriggeredIndexAt(int i);               // Chỉ số ô thứ i trong danh sách đã kích hoạt
void clearTriggeredList();                    // Xóa danh sách sau khi đã gửi MQTT (skipped + completed/late)

#endif
