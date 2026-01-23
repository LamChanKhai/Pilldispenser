#ifndef BUTTON_H
#define BUTTON_H

// Xử lý nút nhấn (INPUT_PULLUP)
// Khi nhấn:
//   1) Nếu Alarm đang kêu → tắt Alarm
//   2) Mở servo thả thuốc xuống

void handleButton();   // gọi liên tục trong loop()

#endif
