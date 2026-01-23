#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== WIFI ====================
extern const char* ssid;
extern const char* password;

// ==================== MQTT ====================
extern const char* mqtt_server;
extern const int   mqtt_port;

extern const char* mqtt_topic_sub;
extern const char* mqtt_topic_config;
extern const char* mqtt_topic_status;
extern const char* mqtt_topic_measurement;
extern const char* USER_ID;
// ==================== I/O =====================
#define LED_PIN 2
#define buttonPin 13

// ============= SERVO DISPENSER ================
#define servo2Pin 18
#define servo2HomeAngle   0
#define servo2ActiveAngle 90

// ============= STEPPER CONFIG =================
#define IN1 14
#define IN2 27
#define IN3 32
#define IN4 33
#define stepsPerCompartment 137
#define numberOfCompartments 15

// ================= AUDIO I2S ===================
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DATA 19
#define AUDIO_FREQ 1200
#define SAMPLE_RATE 16000

// ==================== BOT ====================
#define TELEGRAM_BOT_TOKEN "8317072478:AAG8gkuIb6cPVS9JRoYhonTAGijq_tilO5Q"
#define TELEGRAM_CHAT_ID  "6181402623"

#endif
