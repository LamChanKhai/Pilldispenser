#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "bp.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "audio.h"
#include "time.h"
#define BP_RXD 16
#define BP_TXD -1

HardwareSerial BPSerial(2);
extern PubSubClient client;
extern const char* USER_ID; 
static int bp_sys = 0;
static int bp_dia = 0;
static int bp_pulse = 0;
static char bp_time[32] = {0};
static String line = "";

void initBP() {
    BPSerial.begin(115200, SERIAL_8N1, BP_RXD, BP_TXD);
    Serial.println("🩸 BP module ready");
}

void publishBP() {
    if (bp_sys == 0 || bp_dia == 0 || bp_pulse == 0) return;

    //if (bp_sys > 140 || bp_dia > 90) {
        sendTelegramAlert(bp_sys, bp_dia, bp_pulse);
    //}

    DynamicJsonDocument doc(256);
    doc["userId"] = USER_ID;
    doc["type"]  = "blood_pressure";
    doc["sys"]   = bp_sys;
    doc["dia"]   = bp_dia;
    doc["pulse"] = bp_pulse;
    doc["time"]  = bp_time;

    String payload;
    serializeJson(doc, payload);

    client.publish(mqtt_topic_measurement, payload.c_str());
    beepOnce();  // Bíp khi gửi BP xong

    Serial.println("📤 BP sent:");
    Serial.println(payload);

    bp_sys = bp_dia = bp_pulse = 0;
    memset(bp_time, 0, sizeof(bp_time));
}

void handleBPSerial() {
    while (BPSerial.available()) {
        char c = BPSerial.read();

        if (c == '\n' || c == '\r') {
            line.trim();   // 🔥 QUAN TRỌNG

            if (line.startsWith("sys")) {
                bp_sys = line.substring(line.indexOf('=') + 1).toInt();
            }
            else if (line.startsWith("dia")) {
                bp_dia = line.substring(line.indexOf('=') + 1).toInt();
            }
            else if (line.startsWith("pulse")) {
                bp_pulse = line.substring(line.indexOf('=') + 1).toInt();
            }
            else if (line.startsWith("record_time")) {
                String t = line.substring(line.indexOf(':') + 1);
                t.trim();
                t.toCharArray(bp_time, sizeof(bp_time));
            }

            line = "";

            // ĐỦ 1 RECORD → GỬI
            if (bp_sys && bp_dia && bp_pulse && bp_time[0]) {
                publishBP();
            }
        } else {
            line += c;
        }
    }
}

void sendTelegramAlert(int sys, int dia, int pulse) {
    WiFiClientSecure client;
    client.setInsecure();  // bỏ SSL check

    HTTPClient https;

    String message =
        "🚨 CẢNH BÁO HUYẾT ÁP CAO\n"
        "SYS: " + String(sys) + " mmHg\n"
        "DIA: " + String(dia) + " mmHg\n"
        "PULSE: " + String(pulse) + " bpm";

    String url =
        "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN
        "/sendMessage?chat_id=" TELEGRAM_CHAT_ID
        "&text=" + message;

    url.replace(" ", "%20");
    url.replace("\n", "%0A");
    
    Serial.println(url);

    https.begin(client, url);
    int httpCode = https.GET();
    https.end();

    if (httpCode == 200) {
        Serial.println("📨 Telegram alert sent successfully");
        beepOnce();  // Bíp khi gửi Telegram xong
    } else {
        Serial.printf("❌ Telegram alert failed, code: %d\n", httpCode);
    }
}

void sendTelegramPillTaken() {
    WiFiClientSecure client;
    client.setInsecure();  // bỏ SSL check

    HTTPClient https;

    // Lấy thời gian hiện tại
    struct tm timeinfo;
    String timeStr = "";
    if (getLocalTime(&timeinfo)) {
        char timeBuffer[32];
        strftime(timeBuffer, sizeof(timeBuffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        timeStr = String(timeBuffer);
    }

    String message =
        "✅ ĐÃ LẤY THUỐC\n"
        "Thời gian: " + timeStr + "\n"
        "Người dùng đã nhấn nút và lấy thuốc thành công.";

    String url =
        "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN
        "/sendMessage?chat_id=" TELEGRAM_CHAT_ID
        "&text=" + message;

    url.replace(" ", "%20");
    url.replace("\n", "%0A");
    
    Serial.println("📤 Sending pill taken notification to Telegram...");

    https.begin(client, url);
    int httpCode = https.GET();
    https.end();

    if (httpCode == 200) {
        Serial.println("📨 Telegram notification sent successfully");
    } else {
        Serial.printf("❌ Telegram notification failed, code: %d\n", httpCode);
    }
}

void sendTelegramPillNotTaken() {
    WiFiClientSecure client;
    client.setInsecure();  // bỏ SSL check

    HTTPClient https;

    // Lấy thời gian hiện tại
    struct tm timeinfo;
    String timeStr = "";
    if (getLocalTime(&timeinfo)) {
        char timeBuffer[32];
        strftime(timeBuffer, sizeof(timeBuffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        timeStr = String(timeBuffer);
    }

    String message =
        "⚠️ CHƯA UỐNG THUỐC\n"
        "Thời gian: " + timeStr + "\n"
        "Alarm đã kêu 5 phút nhưng chưa có ai bấm nút lấy thuốc.\n"
        "Vui lòng kiểm tra và lấy thuốc ngay!";

    String url =
        "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN
        "/sendMessage?chat_id=" TELEGRAM_CHAT_ID
        "&text=" + message;

    url.replace(" ", "%20");
    url.replace("\n", "%0A");
    
    Serial.println("📤 Sending pill not taken notification to Telegram...");

    https.begin(client, url);
    int httpCode = https.GET();
    https.end();

    if (httpCode == 200) {
        Serial.println("📨 Telegram notification sent successfully");
    } else {
        Serial.printf("❌ Telegram notification failed, code: %d\n", httpCode);
    }
}