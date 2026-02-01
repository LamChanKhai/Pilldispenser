#include <WiFi.h>          
#include <WiFiClient.h> 
#include <PubSubClient.h>

// ==== IMPORT MODULE TÁCH RIÊNG ====
#include "config.h"
#include "motor.h"
#include "audio.h"
#include "schedule.h"
#include "button.h"
#include "bp.h"
//#include "max3010.h"
#include "time.h"
// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);
// Cấu hình múi giờ Việt Nam (UTC+7)
const long  gmtOffset_sec = 25200;  // 7 * 3600
const int   daylightOffset_sec = 0; // Việt Nam không có giờ mùa đông/hè
const char* ntpServer = "pool.ntp.org"; // Server quốc tế
bool mqttOnline = false;
unsigned long lastMqttAttempt = 0;


// ----------------------------------------------------------------------------------
// 📩 MQTT CALLBACK – nhận lệnh từ server (schedule / dispense / config…)
// ----------------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) msg += (char)payload[i];

    if (String(topic) == mqtt_topic_sub)     setSchedule(msg);   // App gửi lịch uống thuốc
    if (String(topic) == mqtt_topic_config) {
        if (msg == "dispense") dispensePill();  // Cấp thuốc thủ công qua app
        if (msg == "status")   Serial.println("STATUS REQUESTED");
    }
}

// ----------------------------------------------------------------------------------
// 🔄 Tự reconnect nếu MQTT mất kết nối
// ----------------------------------------------------------------------------------
void reconnectMQTT() {
    if (client.connected()) return;

    // chỉ thử reconnect mỗi 3 giây
    if (millis() - lastMqttAttempt < 3000) return;
    lastMqttAttempt = millis();

    Serial.println("MQTT lost → reconnecting...");

    if (client.connect("ESP32PillDispenser",
                       mqtt_topic_status, 0, true,
                       "{\"status\":\"offline\"}")) {

        Serial.println("MQTT reconnected ✓");
        client.subscribe(mqtt_topic_sub);
        client.subscribe(mqtt_topic_config);
        client.publish(mqtt_topic_status, "online", true);

    } else {
        Serial.print("MQTT failed, rc=");
        Serial.println(client.state());
    }
}

// ----------------------------------------------------------------------------------
// 🚀 SETUP – chỉ chạy một lần
// ----------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(300);

    pinMode(buttonPin, INPUT_PULLUP);   // <<< NÚT để tắt chuông + nhả thuốc
    pinMode(LED_PIN, OUTPUT);

    motorInit();
    servo2Init();
    initAudioAlarm();
    clearSchedule();  // Lịch trống ban đầu
    initBP();
    //initMAX3010();
    // ==== WIFI ====
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nWiFi Connected ✓");
    // ==== NTP ====
    // Bắt đầu đồng bộ giờ
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Waiting for time sync...");
    struct tm timeinfo;
    while(!getLocalTime(&timeinfo)){
        Serial.print(".");
        delay(500);
    }
      
    Serial.println("\nTime synced successfully!");
    // ==== MQTT ====
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);

    Serial.println("\n=== SYSTEM READY – CHỜ LỆNH ===");
}

// ----------------------------------------------------------------------------------
// 🔄 LOOP – chạy liên tục
// ----------------------------------------------------------------------------------
void loop() {

    reconnectMQTT();
    client.loop();  

    //Serial.printf("Looping\n");
    handleButton();       // Bấm nút → tắt chuông + bật Servo
    updateAlarmSound();   // Tiếp tục cảnh báo nếu chưa có người bấm
    checkSchedule(dispensePill); // Đến giờ → quay Stepper + bật chuông
    
    // Gọi measureAndPublish() mỗi lần loop để state machine hoạt động non-blocking
    // Hàm này sẽ tự quản lý timing và chỉ bắt đầu đo mới sau khi hoàn thành đo trước đó
    
    //measureAndPublish();
    handleBPSerial();   // đọc dữ liệu huyết áp liên tục

    delay(20);
}
