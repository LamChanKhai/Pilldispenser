#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "config.h"
#include "max3010.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "spo2_algorithm.h"
#include "MAX30105.h"
#include "heartRate.h"

// ================= SENSOR OBJECT =================
MAX30105 sensor;

extern PubSubClient client;

// ================= RAW BUFFERS =================
uint32_t irBuff[100];    // 100 sample ~1s
uint32_t redBuff[100];

bool sensorReady = false;

// ================= STATE MACHINE =================
enum MeasurementState {
    IDLE,                    // Chờ bắt đầu đo
    WAITING_FOR_FINGER,      // Đang chờ ngón tay
    COLLECTING_DATA          // Đang thu thập dữ liệu
};

static MeasurementState state = IDLE;
static int sampleIndex = 0;
static unsigned long measurementStartTime = 0;
static unsigned long collectingStartTime = 0;  // Thời gian bắt đầu thu thập
static unsigned long lastSampleTime = 0;
static unsigned long lastMeasurementEndTime = 0;
static int consecutiveLowSamples = 0;  // Đếm số sample thấp liên tiếp
static const unsigned long MEASUREMENT_INTERVAL = 3000; // Đo mỗi 3 giây
static const unsigned long WAITING_TIMEOUT = 10000; // Timeout chờ ngón tay: 10 giây
static const unsigned long COLLECTING_TIMEOUT = 3000; // Timeout thu thập: 3 giây (đủ cho 100 samples)
static const int MAX_LOW_SAMPLES = 5; // Cho phép 5 sample thấp liên tiếp

// =================================================
// INIT SENSOR
// =================================================
bool initMAX3010() {
    Wire.begin(21, 22);      // SDA=21 , SCL=22 🔥 bạn yêu cầu

    Serial.println("🔍 Initializing MAX3010x ...");

    if (!sensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println("❌ MAX3010x Not Found!");
        return false;
    }

    sensor.setup(0x1F, 4, 2, 100, 411, 4096); // cấu hình tối ưu

    sensor.setPulseAmplitudeRed(0x1F);
    sensor.setPulseAmplitudeIR(0x1F);
    sensor.clearFIFO();

    sensorReady = true;
    Serial.println("✅ MAX3010x Ready");
    return true;
}


// =================================================
// NON-BLOCKING MEASUREMENT + ANALYZE + SEND MQTT
// =================================================
void measureAndPublish() {
    if (!sensorReady) return;

    unsigned long now = millis();

    // State machine để không block code
    switch (state) {
        case IDLE:
            // Chỉ bắt đầu đo mới nếu đã đủ thời gian từ lần đo trước
            if (now - lastMeasurementEndTime >= MEASUREMENT_INTERVAL) {
                state = WAITING_FOR_FINGER;
                measurementStartTime = now;
                sampleIndex = 0;
                Serial.println("🩺 Starting measurement...");
            }
            break;

        case WAITING_FOR_FINGER: {
            // Kiểm tra timeout
            if (now - measurementStartTime > WAITING_TIMEOUT) {
                Serial.println("⚠ Measurement timeout (no finger?)");
                state = IDLE;
                lastMeasurementEndTime = now; // Reset để có thể đo lại ngay
                return;
            }

            // Đọc giá trị cảm biến
            uint32_t ir = sensor.getIR();
            uint32_t red = sensor.getRed();

            // Nếu phát hiện ngón tay, chuyển sang thu thập dữ liệu
            if (ir >= 5000) {
                state = COLLECTING_DATA;
                collectingStartTime = now;  // Reset timeout cho việc thu thập
                lastSampleTime = now;
                consecutiveLowSamples = 0;  // Reset counter
                irBuff[0] = ir;
                redBuff[0] = red;
                sampleIndex = 1;
                Serial.println("👆 Finger detected, collecting data...");
            }
            break;
        }

        case COLLECTING_DATA: {
            // Kiểm tra timeout từ khi bắt đầu thu thập (không phải từ khi chờ ngón tay)
            if (now - collectingStartTime > COLLECTING_TIMEOUT) {
                Serial.printf("⚠ Collection timeout (collected %d/100 samples)\n", sampleIndex);
                state = IDLE;
                lastMeasurementEndTime = now;
                return;
            }

            // Thu thập sample mỗi 10ms (~100Hz)
            if (now - lastSampleTime >= 10) {
                uint32_t ir = sensor.getIR();
                uint32_t red = sensor.getRed();

                // Kiểm tra ngón tay với tolerance (cho phép một vài sample thấp)
                if (ir < 5000) {
                    consecutiveLowSamples++;
                    // Chỉ coi là mất ngón tay nếu có nhiều sample thấp liên tiếp
                    if (consecutiveLowSamples >= MAX_LOW_SAMPLES) {
                        Serial.printf("⚠ Finger removed (collected %d/100 samples)\n", sampleIndex);
                        state = WAITING_FOR_FINGER;
                        measurementStartTime = now; // Reset timeout
                        consecutiveLowSamples = 0;
                        break;
                    }
                    // Nếu chỉ là một vài sample thấp, vẫn tiếp tục nhưng không lưu
                    // (có thể là nhiễu tạm thời)
                } else {
                    // Reset counter nếu có giá trị tốt
                    consecutiveLowSamples = 0;
                    
                    // Lưu sample vào buffer
                    irBuff[sampleIndex] = ir;
                    redBuff[sampleIndex] = red;
                    sampleIndex++;
                    lastSampleTime = now;

                    // Debug mỗi 20 samples
                    if (sampleIndex % 20 == 0) {
                        Serial.printf("📈 Progress: %d/100 samples\n", sampleIndex);
                    }

                    // Đã thu thập đủ 100 samples
                    if (sampleIndex >= 100) {
                        Serial.println("✅ Collected 100 samples, calculating...");
                        
                        // ===== TÍNH TOÁN =====
                        int32_t spo2 = 0, heartRate = 0;
                        int8_t  validSPO2 = 0, validHR = 0;

                        maxim_heart_rate_and_oxygen_saturation(
                            irBuff, 100, redBuff,
                            &spo2, &validSPO2,
                            &heartRate, &validHR
                        );

                        if (!validHR || !validSPO2) {
                            Serial.println("⚠ Invalid reading - try again");
                            state = IDLE;
                            lastMeasurementEndTime = now;
                            return;
                        }

                        Serial.printf("📊 HR=%d bpm | SpO2=%d%%\n", heartRate, spo2);

                        // ===== GỬI MQTT =====
                        DynamicJsonDocument doc(256);
                        doc["heart"] = heartRate;
                        doc["spo2"]  = spo2;
                        doc["ts"]    = millis();

                        String payload;
                        serializeJson(doc, payload);

                        client.publish(mqtt_topic_measurement, payload.c_str());
                        Serial.println("📤 MQTT sent: " + payload);

                        // Reset về trạng thái idle và lưu thời gian kết thúc
                        lastMeasurementEndTime = now;
                        state = IDLE;
                        sampleIndex = 0;
                    }
                }
            }
            break;
        }
    }
}