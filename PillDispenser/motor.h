#ifndef MOTOR_H
#define MOTOR_H

// ======== STEPPER MOTOR CONTROL =========
void motorInit();                     // Khởi tạo chân motor
void rotateStepperMotor(int steps);   // Quay stepper
void rotateToNextCompartment();       // Quay đúng 1 ngăn

// ======== SERVO DISPENSER CONTROL =======
void servo2Init();                    // Khởi tạo Servo2 (cửa thả thuốc)
void triggerServo2();                 // Mở cửa thả
void updateServo2();                  // Tự đóng sau 500ms

// ======== DISPENSE ACTION ===============
// logic: quay motor → bật Alarm → chờ bấm nút để thả thuốc
void dispensePill();

#endif
