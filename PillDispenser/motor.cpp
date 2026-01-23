#include <Arduino.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

#include "motor.h"
#include "config.h"
#include "audio.h"   // để bật báo khi đến giờ uống thuốc

// ========================= SERVO VARIABLES =========================
Servo myServo2;
bool servo2Holding = false;
unsigned long servo2ActivatedAt = 0;
const unsigned long servo2HoldDuration = 500; // ms giữ cửa mở → 0.5s

// ========================= STEPPER VARIABLES =========================
int currentCompartment = 0;  // ngăn thuốc đang đứng

// 28BYJ-48 half-step: 4096 steps = 360°
#define MotorInterfaceType 8
#define STEPS_180 2048   // 👈 180 độ CHUẨN

// Initialize stepper (IN1-IN3-IN2-IN4)
AccelStepper stepper = AccelStepper(MotorInterfaceType, IN1, IN3, IN2, IN4);

// ====================================================================
// INIT MOTOR PINS
// ====================================================================
void motorInit() {
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    stepper.setMaxSpeed(800);
    stepper.setAcceleration(400);
    stepper.setCurrentPosition(0);
}

// ====================================================================
// LOW-LEVEL STEP MOTOR MOVE (BLOCKING)
// ====================================================================
void rotateStepperMotor(int steps) {
    stepper.enableOutputs();

    stepper.setCurrentPosition(0);
    stepper.moveTo(steps);

    while (stepper.distanceToGo() != 0) {
        stepper.run();   // 👈 BẮT BUỘC
    }

    stepper.disableOutputs();
}

// ====================================================================
// QUAY 180 ĐỘ (1/2 VÒNG)
// ====================================================================
void rotateToNextCompartment() {
    rotateStepperMotor(STEPS_180);   // 👈 XOAY 180°

    currentCompartment++;
    if (currentCompartment >= numberOfCompartments)
        currentCompartment = 0;

    Serial.println("Stepper rotated 180°");
}

// ====================================================================
// SERVO CONTROL – CHỈ KÍCH HOẠT KHI NGƯỜI BẤM NÚT
// ====================================================================
void servo2Init() {
    myServo2.attach(servo2Pin);
    myServo2.write(servo2HomeAngle);
}

void triggerServo2() {
    if (servo2Holding) return;

    myServo2.write(servo2ActiveAngle);
    servo2Holding = true;
    servo2ActivatedAt = millis();
    Serial.println("Servo2 → OPEN");
}

void updateServo2() {
    if (servo2Holding && millis() - servo2ActivatedAt >= servo2HoldDuration) {
        myServo2.write(servo2HomeAngle);
        servo2Holding = false;
        Serial.println("Servo2 → CLOSE");
    }
}

// ====================================================================
// 📌 DISPENSE LOGIC
// ====================================================================
void dispensePill() {
    rotateToNextCompartment(); // 👈 xoay 180°
    startAlarmSound();
    Serial.println("📢 Waiting for user confirmation (press button)");
}
