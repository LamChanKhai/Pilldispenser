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
#include "audio.h"

// ================= SENSOR OBJECT =================
MAX30105 sensor;

extern PubSubClient client;
extern const char* USER_ID;

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
static const unsigned long COLLECTING_TIMEOUT = 10000; // Timeout thu thập: 10 giây (dư cho 100 samples, tránh timeout sớm)
static const int MAX_LOW_SAMPLES = 5; // Cho phép 5 sample thấp liên tiếp

// Phát hiện ngón tay: dùng baseline để so sánh tương đối
static int consecutiveFingerSamples = 0;
static const int REQUIRED_FINGER_SAMPLES = 20;      // ~200ms nếu mỗi 10ms một mẫu
static uint32_t baselineIR = 0;                      // Giá trị nền IR (tính khi khởi động)
static const float FINGER_INCREASE_RATIO = 1.15f;    // Ngón tay phải tăng >= 15% so với baseline
static bool baselineCalculated = false;

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
    
    // Tính baseline IR (giá trị nền khi không có ngón tay)
    Serial.println("📊 Calculating baseline IR...");
    delay(500); // Đợi sensor ổn định
    uint32_t sumIR = 0;
    for (int i = 0; i < 50; i++) {
        sumIR += sensor.getIR();
        delay(20);
    }
    baselineIR = sumIR / 50;
    baselineCalculated = true;
    Serial.printf("📊 Baseline IR: %lu (no finger)\n", (unsigned long)baselineIR);
    
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
                lastSampleTime = now;
                sampleIndex = 0;
                consecutiveFingerSamples = 0;
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

            // Lấy mẫu mỗi 10ms để tránh đọc quá dày
            if (now - lastSampleTime >= 10) {
                lastSampleTime = now;

                // Đọc giá trị cảm biến
                uint32_t ir = sensor.getIR();
                uint32_t red = sensor.getRed();

                // Tính baseline nếu chưa tính (fallback)
                if (!baselineCalculated || baselineIR == 0) {
                    baselineIR = ir;
                    baselineCalculated = true;
                    Serial.printf("📊 Baseline IR set: %lu\n", (unsigned long)baselineIR);
                }

                // Debug nhẹ để xem giá trị nền (in thưa để khỏi spam)
                static uint8_t debugCnt = 0;
                if (++debugCnt >= 50) { // ~500ms
                    debugCnt = 0;
                    float ratio = baselineIR > 0 ? (float)ir / baselineIR : 0;
                    Serial.printf("👀 Waiting: IR=%lu (baseline=%lu, ratio=%.2f), fingerCnt=%d\n",
                                  (unsigned long)ir, (unsigned long)baselineIR, ratio, consecutiveFingerSamples);
                }

                // Kiểm tra có ngón tay: IR phải tăng đáng kể so với baseline (>=15%)
                uint32_t fingerThreshold = (uint32_t)(baselineIR * FINGER_INCREASE_RATIO);
                if (ir >= fingerThreshold) {
                    consecutiveFingerSamples++;
                    if (consecutiveFingerSamples >= REQUIRED_FINGER_SAMPLES) {
                        state = COLLECTING_DATA;
                        collectingStartTime = now;  // Reset timeout cho việc thu thập
                        lastSampleTime = now;
                        consecutiveLowSamples = 0;  // Reset counter

                        beepOnce();  // Bíp khi bắt đầu đo

                        // Lưu sample đầu tiên
                        irBuff[0] = ir;
                        redBuff[0] = red;
                        sampleIndex = 1;
                        Serial.printf("👆 Finger detected (stable): IR=%lu (baseline=%lu, +%.1f%%), collecting data...\n",
                                     (unsigned long)ir, (unsigned long)baselineIR, 
                                     ((float)ir / baselineIR - 1.0f) * 100.0f);
                    }
                } else {
                    // Nếu tụt xuống dưới ngưỡng thì reset đếm
                    if (consecutiveFingerSamples > 0) {
                        Serial.printf("ℹ️ Finger candidate reset: IR=%lu (need >=%lu)\n", 
                                     (unsigned long)ir, (unsigned long)fingerThreshold);
                    }
                    consecutiveFingerSamples = 0;
                }
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
                lastSampleTime = now;

                uint32_t ir = sensor.getIR();
                uint32_t red = sensor.getRed();

                // Luôn lưu sample để đảm bảo đủ 100 mẫu cho thuật toán
                if (sampleIndex < 100) {
                    irBuff[sampleIndex] = ir;
                    redBuff[sampleIndex] = red;
                    sampleIndex++;

                    // Phát hiện mất ngón tay: IR tụt về gần baseline (giảm <10% so với baseline)
                    uint32_t fingerLostThreshold = (uint32_t)(baselineIR * 1.05f); // Chỉ cao hơn baseline 5%
                    if (ir < fingerLostThreshold) {
                        consecutiveLowSamples++;
                        if (consecutiveLowSamples >= MAX_LOW_SAMPLES) {
                            Serial.printf("⚠ Finger removed during collection (collected %d/100 samples, IR=%lu)\n", 
                                         sampleIndex, (unsigned long)ir);
                            state = WAITING_FOR_FINGER;
                            measurementStartTime = now; // Reset timeout chờ lại
                            consecutiveLowSamples = 0;
                            sampleIndex = 0;
                            consecutiveFingerSamples = 0;
                            break;
                        }
                    } else {
                        consecutiveLowSamples = 0;
                    }

                    // Debug mỗi 20 samples
                    if (sampleIndex % 20 == 0) {
                        Serial.printf("📈 Progress: %d/100 samples\n", sampleIndex);
                    }
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

                    // ===== ĐỌC NHIỆT ĐỘ TỪ MAX3010x =====
                    // Thư viện MAX30105 cung cấp hàm đọc nhiệt độ nội bộ của cảm biến (°C)
                    float temperature = sensor.readTemperature();
                    Serial.printf("🌡 Temperature=%.2f°C\n", temperature);

                    // ===== GỬI MQTT (chỉ SpO2 + nhiệt độ) =====
                    DynamicJsonDocument doc(256);
                    doc["spo2"] = spo2;
                    doc["temp"] = temperature;   // gửi nhiệt độ (°C)
                    doc["ts"]   = millis();
                    doc["type"] = "spo2"; 
                    doc["userId"] = USER_ID;
                    String payload;
                    serializeJson(doc, payload);

                    client.publish(mqtt_topic_measurement, payload.c_str());
                    Serial.println("📤 MQTT sent: " + payload);

                    beepOnce();  // Bíp khi đo xong
                    beepOnce();
                    // Reset về trạng thái idle và lưu thời gian kết thúc
                    lastMeasurementEndTime = now;
                    state = IDLE;
                    sampleIndex = 0;
                }
            }
            break;
        }
    }
}