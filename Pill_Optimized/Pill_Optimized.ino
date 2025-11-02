#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <time.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ========== USER ID ==========
const char* userId = "690583a8d3dde5f187c1cd58";

// ========== CONFIGURATION ==========
// WiFi Configuration
const char* ssid = "LCK";
const char* password = "123456790";

// MQTT Configuration
const char* mqtt_server = "34.63.156.115";
const int mqtt_port = 1883;
const char* mqtt_topic_sub = "pill/command/schedule";
const char* mqtt_topic_pub = "pill/data/log";
const char* mqtt_topic_status = "pill/data/status";
const char* mqtt_topic_config = "pill/command/config";
const char* mqtt_topic_measurement = "pill/data/measurement";

// Hardware Configuration
const int servoPin = 5;
const int ledPin = 2;
const int servoOpenAngle = 30;
const int servoCloseAngle = 0;


// NTP Configuration
const char* ntpServer = "time.nist.gov";
const long gmtOffset_sec = 7 * 3600;  // Vietnam timezone GMT+7
const int daylightOffset_sec = 0;

// Author
const char* author = "Lam Chan Khai";
const char* author_email = "ckhai.lam05@gmail.com";
// ========== GLOBAL VARIABLES ==========
Servo myServo;
WiFiClient espClient;
PubSubClient client(espClient);
MAX30105 particleSensor;

// MAX3010x measurement variables
bool measuring = false;
const int MEASUREMENT_BUFFER_SIZE = 100;
uint32_t irBuffer[MEASUREMENT_BUFFER_SIZE];
uint32_t redBuffer[MEASUREMENT_BUFFER_SIZE];
int bufferIndex = 0;

// Heart rate calculation variables
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// SpO2 and Heart Rate results
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// Schedule management
struct ScheduleEntry {
  char time[6];  // HH:MM format
  bool active;
};

ScheduleEntry schedule[12];
int scheduleCount = 0;
int currentScheduleIndex = 0;
unsigned long lastDispenseTime = 0;
const unsigned long dispenseCooldown = 60000; // 1 minute cooldown

// System status
bool systemOnline = false;
bool ntpSynced = false;
unsigned long lastStatusUpdate = 0;
const unsigned long statusUpdateInterval = 30000; // 30 seconds

// ========== UTILITY FUNCTIONS ==========
void logMessage(const char* message) {
  Serial.println(message);
  if (client.connected()) {
    client.publish(mqtt_topic_pub, message);
  }
}

void setLED(bool state) {
  digitalWrite(ledPin, state);
}

void blinkLED(int times = 3, int delay_ms = 200) {
  for (int i = 0; i < times; i++) {
    setLED(HIGH);
    delay(delay_ms);
    setLED(LOW);
    delay(delay_ms);
  }
}

// ========== SCHEDULE MANAGEMENT ==========
void clearSchedule() {
  for (int i = 0; i < 12; i++) {
    schedule[i].active = false;
    memset(schedule[i].time, 0, sizeof(schedule[i].time));
  }
  scheduleCount = 0;
  currentScheduleIndex = 0;
}

void addScheduleEntry(const String& timeStr) {
  if (scheduleCount >= 12) return;
  
  timeStr.toCharArray(schedule[scheduleCount].time, 6);
  schedule[scheduleCount].active = true;
  scheduleCount++;
  
  Serial.printf("📌 Added schedule: %s (Total: %d)\n", timeStr.c_str(), scheduleCount);
}

void setSchedule(const String& payload) {
  clearSchedule();
  Serial.printf("📥 Received payload: %s\n", payload.c_str());

  // Parse JSON if possible, otherwise fallback to CSV
  if (payload.startsWith("{")) {
    parseJsonSchedule(payload);
  } else {
    parseCsvSchedule(payload);
  }
  
  // Sort schedule by time
  //sortSchedule();
  // Send confirmation
  char confirmMsg[100];
  for (int i = 0; i < scheduleCount; i++) {
    Serial.printf("Schedule %d: %s\n", i + 1, schedule[i].time);
  }
  snprintf(confirmMsg, sizeof(confirmMsg), "Schedule set with %d entries", scheduleCount);
  logMessage(confirmMsg);
}

void parseJsonSchedule(const String& json) {
  DynamicJsonDocument doc(1024);
  Serial.println("1");
  if (deserializeJson(doc, json) == DeserializationError::Ok) {
    String mode = doc["mode"];
    Serial.println("2");
    if (mode == "quick" && doc.containsKey("times")) {
      JsonArray times = doc["times"];
      for (int i = 0; i < times.size() && scheduleCount < 12; i++) {
        addScheduleEntry(times[i].as<String>());
      }
    } else if (mode == "custom" && doc.containsKey("thoiGian")) {
      JsonObject times = doc["thoiGian"];
      Serial.println(times.size());
      for (JsonPair kv : times) {
         JsonObject entry = kv.value().as<JsonObject>();
         String gio = entry["gio"].as<String>();
         addScheduleEntry(gio);
      }
    }
  }
}

void parseCsvSchedule(const String& csv) {
  int start = 0, end;
  String tokens[15];
  int tokenCount = 0;

  // Parse CSV
  while ((end = csv.indexOf(',', start)) != -1 && tokenCount < 15) {
    tokens[tokenCount++] = csv.substring(start, end);
    start = end + 1;
  }
  tokens[tokenCount++] = csv.substring(start);

  if (tokenCount < 2) return;

  String mode = tokens[0];
  mode.trim();

  if (mode == "quick" && tokenCount >= 3) {
    String t1 = tokens[1];
    String t2 = tokens[2];
    for (int i = 0; i < 12; i++) {
      addScheduleEntry((i % 2 == 0) ? t1 : t2);
    }
  } else if (mode == "custom") {
    for (int i = 1; i < tokenCount && i <= 12; i++) {
      addScheduleEntry(tokens[i]);
    }
  }
}

void sortSchedule() {
  // Simple bubble sort by time
  for (int i = 0; i < scheduleCount - 1; i++) {
    for (int j = 0; j < scheduleCount - i - 1; j++) {
      if (strcmp(schedule[j].time, schedule[j + 1].time) > 0) {
        ScheduleEntry temp = schedule[j];
        schedule[j] = schedule[j + 1];
        schedule[j + 1] = temp;
      }
    }
  }
}

// ========== SERVO CONTROL ==========
void dispensePill() {
  if (millis() - lastDispenseTime < dispenseCooldown) {
    Serial.println("⏳ Cooldown active, skipping dispense");
    return;
  }

  Serial.println("💊 Dispensing pill...");
  blinkLED(2, 100);
  
  myServo.write(servoOpenAngle);
  
  lastDispenseTime = millis();
  logMessage("💊 Pill dispensed successfully");
}

// ========== TIME MANAGEMENT ==========
bool getCurrentTime(char* timeStr) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  return true;
}

void syncNTP() {
  if (ntpSynced) return;
  
  Serial.println("⏳ Syncing NTP...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(2000);
    attempts++;
  }
  
  if (attempts < 10) {
    ntpSynced = true;
    Serial.println("✅ NTP synchronized successfully");
  } else {
    Serial.println("❌ NTP sync failed");
  }
}

// ========== MQTT FUNCTIONS ==========
void mqttCallback(char* topic, byte* message, unsigned int length) {
  String messageStr;
  for (int i = 0; i < length; i++) {
    messageStr += (char)message[i];
  }
  
  Serial.printf("📨 MQTT message on %s: %s\n", topic, messageStr.c_str());
  
  if (String(topic) == mqtt_topic_sub) {
    setSchedule(messageStr);
  } else if (String(topic) == mqtt_topic_config) {
    handleConfigMessage(messageStr);
  }
}

void handleConfigMessage(const String& message) {
  if (message == "status") {
    sendStatusReport();
  } else if (message == "dispense") {
    dispensePill();
  } else if (message == "reset") {
    ESP.restart();
  }
}

void sendStatusReport() {
  DynamicJsonDocument doc(512);
  doc["status"] = systemOnline ? "online" : "offline";
  doc["ntp_synced"] = ntpSynced;
  doc["schedule_count"] = scheduleCount;
  doc["current_index"] = currentScheduleIndex;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["author"] = author;
  doc["author_email"] = author_email;
  
  char currentTime[6];
  if (getCurrentTime(currentTime)) {
    doc["current_time"] = currentTime;
  }
  
  String statusJson;
  serializeJson(doc, statusJson);
  client.publish(mqtt_topic_status, statusJson.c_str());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("🔄 Connecting to MQTT...");
    
    if (client.connect("ESP32PillDispenser", mqtt_topic_status, 0, true, "{\"status\":\"offline\"}")) {
      Serial.println("✅ Connected!");
      
      // Subscribe to topics
      client.subscribe(mqtt_topic_sub);
      client.subscribe(mqtt_topic_config);
      
      // Send online status
      client.publish(mqtt_topic_status, "online", true);
      systemOnline = true;
      
      // Send initial status report
      sendStatusReport();
      
      blinkLED(1, 500);
    } else {
      Serial.printf("❌ Failed, error code: %d\n", client.state());
      delay(2000);
    }
  }
}

// ========== MAX3010x FUNCTIONS ==========
bool initializeMAX3010x() {
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("❌ MAX3010x not found. Please check wiring/power.");
    return false;
  }
  
  Serial.println("✅ MAX3010x found!");
  
  // Configure sensor
  byte ledBrightness = 0x1F; // Options: 0=Off to 255=50mA
  byte sampleAverage = 4; // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100; // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // Options: 69, 118, 215, 411
  int adcRange = 4096; // Options: 2048, 4096, 8192, 16384
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);
  
  // Initialize buffers
  bufferIndex = 0;
  lastBeat = 0;
  beatAvg = 0;
  
  return true;
}

void startMeasurement() {
  if (measuring) {
    return;
  }
  
  Serial.println("🫀 Starting measurement...");
  measuring = true;
  bufferIndex = 0;
  lastBeat = 0;
  
  // Clear buffers
  for (int i = 0; i < MEASUREMENT_BUFFER_SIZE; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
  }
  
  // Reset heart rate variables
  for (byte x = 0; x < RATE_SIZE; x++) {
    rates[x] = 0;
  }
  rateSpot = 0;
  
  blinkLED(2, 100);
}

void collectMeasurementData() {
  if (!measuring) return;
  
  // Read sensor values
  uint32_t irValue = particleSensor.getIR();
  uint32_t redValue = particleSensor.getRed();
  
  // Debug: print values occasionally
  if (bufferIndex % 20 == 0) {
    Serial.printf("📊 IR: %lu, Red: %lu, Progress: %d/%d\n", irValue, redValue, bufferIndex, MEASUREMENT_BUFFER_SIZE);
  }
  
  if (irValue < 50000) {
    // Finger removed during measurement
    if (bufferIndex > 10) {
      Serial.println("⚠️ Finger removed - Resetting measurement");
      measuring = false;
      bufferIndex = 0;
    }
    return;
  }
  
  // Store in buffers
  if (bufferIndex < MEASUREMENT_BUFFER_SIZE) {
    irBuffer[bufferIndex] = irValue;
    redBuffer[bufferIndex] = redValue;
    bufferIndex++;
    
    // Check for heartbeat
    if (checkForBeat((long)irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }
    }
  } else {
    // Buffer full, calculate SpO2 and Heart Rate
    Serial.println("🔄 Calculating results...");
    maxim_heart_rate_and_oxygen_saturation(irBuffer, MEASUREMENT_BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    
    if (validSPO2 && validHeartRate) {
      sendMeasurementData(beatAvg > 0 ? beatAvg : heartRate, spo2, particleSensor.readTemperature());
      Serial.printf("✅ Measurement complete: HR=%d, SpO2=%d%%, Temp=%.1f°C\n", 
                   beatAvg > 0 ? beatAvg : heartRate, spo2, particleSensor.readTemperature());
    } else {
      Serial.println("❌ Invalid measurement, please try again");
    }
    
    measuring = false;
    bufferIndex = 0;
  }
}

void sendMeasurementData(int heartRate, int spo2, float temperature) {
  DynamicJsonDocument doc(256);
  doc["userId"] = userId;
  doc["heart_beat"] = heartRate;
  doc["spo2"] = spo2;
  doc["temp"] = temperature;
  doc["timestamp"] = millis();
  
  char currentTime[20];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    sprintf(currentTime, "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    doc["datetime"] = currentTime;
  }
  
  String measurementJson;
  serializeJson(doc, measurementJson);
  
  if (client.connected()) {
    client.publish(mqtt_topic_measurement, measurementJson.c_str());
    Serial.printf("📤 Published to MQTT: %s\n", measurementJson.c_str());
  } else {
    Serial.println("❌ MQTT not connected, measurement not sent");
  }
}

// ========== SCHEDULE CHECKING ==========
void checkSchedule() {
  if (scheduleCount == 0 || !ntpSynced) return;
  
  char currentTime[6];
  if (!getCurrentTime(currentTime)) return;
  
  // Check if current time matches current schedule
  
    if (schedule[currentScheduleIndex].active && strcmp(schedule[currentScheduleIndex].time, currentTime) == 0) {
      Serial.printf("⏰ Time match: %s (Schedule %d/%d)\n", 
                   currentTime, currentScheduleIndex + 1, scheduleCount);
      currentScheduleIndex++;
      dispensePill();
    }
  
}

// ========== MAIN FUNCTIONS ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🚀 Starting Pill Dispenser System...");
  
  // Initialize hardware
  pinMode(ledPin, OUTPUT);
  setLED(LOW);
  
  myServo.attach(servoPin);
  myServo.write(servoCloseAngle);
  
  // Initialize I2C for MAX3010x
  Wire.begin();
  
  // Initialize MAX3010x sensor
  if (!initializeMAX3010x()) {
    Serial.println("⚠️ MAX3010x initialization failed, continuing without sensor");
  }
  
  // Initialize schedule
  clearSchedule();
  
  // Initialize heart rate variables
  for (byte x = 0; x < RATE_SIZE; x++) {
    rates[x] = 0;
  }
  rateSpot = 0;
  lastBeat = 0;
  
  // Connect to WiFi
  Serial.printf("📶 Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    blinkLED(2, 200);
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    return;
  }
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  
  // Sync NTP
  syncNTP();
  
  Serial.println("✅ Setup complete!");
}

void loop() {
  // Handle MQTT
  if (!client.connected()) {
    systemOnline = false;
    reconnectMQTT();
  }
  client.loop();
  
  // Sync NTP periodically
  if (!ntpSynced && millis() % 300000 < 1000) { // Every 5 minutes
    syncNTP();
  }
  
  // Check schedule
  checkSchedule();
  
  // Auto-detect finger and start measurement
  if (!measuring) {
    static unsigned long lastFingerCheck = 0;
    if (millis() - lastFingerCheck > 500) { // Check every 500ms
      uint32_t irValue = particleSensor.getIR();
      Serial.println(irValue);
      if (irValue > 50000) {
        Serial.println("👆 Finger detected - Starting measurement automatically...");
        startMeasurement();
      }
      lastFingerCheck = millis();
    }
  }
  
  // Collect measurement data if measuring
  if (measuring) {
    collectMeasurementData();
    delay(10); // Fast reading for measurement (100Hz)
  } else {
    // Send periodic status updates
    if (millis() - lastStatusUpdate > statusUpdateInterval) {
      sendStatusReport();
      lastStatusUpdate = millis();
    }
    
    // Heartbeat LED
    if (systemOnline && ntpSynced) {
      static unsigned long lastHeartbeat = 0;
      if (millis() - lastHeartbeat > 10000) { // Every 10 seconds
        setLED(HIGH);
        delay(50);
        setLED(LOW);
        lastHeartbeat = millis();
      }
    }
    
    delay(100); // Normal delay when not measuring
  }
}                                                                                                                             

