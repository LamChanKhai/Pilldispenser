#ifndef MAX3010_H
#define MAX3010_H

// ================== KHỞI TẠO CẢM BIẾN ==================
bool initMAX3010();               // gọi trong setup()

// ================== ĐỌC + TÍNH TOÁN + MQTT ==============
void measureAndPublish();         // gọi trong loop()

#endif
