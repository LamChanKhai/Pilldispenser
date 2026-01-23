#include <Arduino.h>
#include "button.h"
#include "config.h"
#include "audio.h"
#include "motor.h"

// ====== DEBOUNCE ======
int lastButtonState   = HIGH;
int stableState       = HIGH;
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 50; // ms

// =======================================================
// 📌 handleButton()
//
// Khi người dùng bấm nút:
//
// 1) Nếu Alarm đang kêu → stopAlarmSound();
// 2) triggerServo2() để thả thuốc;
// =======================================================

void handleButton() {
    int reading = digitalRead(buttonPin);

    // kiểm tra thay đổi tín hiệu nút
    if (reading != lastButtonState) {
        lastDebounce = millis();
        lastButtonState = reading;
    }

    // xác nhận ổn định (debounced)
    if ((millis() - lastDebounce) > debounceDelay) {

        if (reading != stableState) {
            stableState = reading;

            // ===========================
            // BẤM NÚT = LOW (INPUT_PULLUP)
            // ===========================
            if (stableState == LOW) {

                Serial.println("🔘 BUTTON PRESSED");

                // Nếu alarm đang chạy → tắt ngay
                if (isAlarmActive()) {
                    stopAlarmSound();
                    Serial.println("🔇 Alarm stopped");
                }

                // Mở servo thả thuốc xuống
                triggerServo2();
                Serial.println("🔓 Servo OPEN → pill dropped");

            }
        }
    }
}
