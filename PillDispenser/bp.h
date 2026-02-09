#pragma once

void initBP();
void handleBPSerial();
void sendTelegramAlert(int sys, int dia, int pulse);
void sendTelegramPillTaken();  // Gửi thông báo đã lấy thuốc
void sendTelegramPillNotTaken();  // Gửi thông báo chưa uống thuốc