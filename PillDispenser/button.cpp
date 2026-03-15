#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "button.h"
#include "config.h"
#include "audio.h"
#include "motor.h"
#include "bp.h"
#include "time.h"
#include "schedule.h"

extern PubSubClient client;

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
    int reading = !digitalRead(buttonPin);

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

                // Nếu alarm đang chạy → tắt ngay, gửi Telegram và gửi MQTT (completed hoặc late)
                if (isAlarmActive()) {
                    // Xác định trước khi stopAlarmSound() xóa alarmStartTime
                    bool late = isAlarmOverdue();
                    const char* statusStr = late ? "late" : "completed";

                    stopAlarmSound();
                    Serial.println("🔇 Alarm stopped");

                    // Gửi thông báo đã lấy thuốc đến Telegram
                    sendTelegramPillTaken();
                    Serial.println("📨 Telegram notification sent: Pill taken");

                    // Gửi qua MQTT: ô bị skip (không bấm) = skipped, ô vừa bấm = completed/late
                    int idx = getLastTriggeredIndex();
                    if (idx >= 0 && client.connected()) {
                        DynamicJsonDocument doc(512);
                        doc["userId"] = USER_ID;
                        JsonArray updates = doc.createNestedArray("updates");
                        int n = getTriggeredCount();
                        for (int i = 0; i < n; i++) {
                            int slot = getTriggeredIndexAt(i);
                            if (slot < 0) continue;
                            JsonObject u = updates.add<JsonObject>();
                            u["index"] = slot;
                            u["status"] = (slot == idx) ? statusStr : "skipped";
                        }
                        if (updates.isNull() || updates.size() == 0) {
                            JsonObject u = updates.add<JsonObject>();
                            u["index"] = idx;
                            u["status"] = statusStr;
                        }
                        String payload;
                        serializeJson(doc, payload);
                        client.publish(mqtt_topic_schedule, payload.c_str());
                        clearTriggeredList();
                        Serial.printf("📤 Schedule updates sent: %s index=%d (+ %d skipped)\n", statusStr, idx, n - 1);
                    }
                }

                // Mở servo thả thuốc xuống
                triggerServo2();
                Serial.println("🔓 Servo OPEN → pill dropped");

            }
        }
    }
}
