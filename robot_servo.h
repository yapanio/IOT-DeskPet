#ifndef ROBOT_SERVO_H
#define ROBOT_SERVO_H

#include "robot_state.h"
#include <ESP32Servo.h>

class RobotServo {
 private:
  Servo myServo;
  int servoPin;

  float currentAngle = 90.0;
  float targetAngle  = 90.0;
  unsigned long lastUpdate = 0;

  RobotState currentState = NORMAL_HAPPY;

  unsigned long lastNormalHappyMove = 0;

  bool isShaking = false;
  unsigned long shakeEndTime = 0;

 public:
  RobotServo(int pin) : servoPin(pin) {}

  void triggerGentleShake() {
    isShaking    = true;
    shakeEndTime = millis() + 1000;
    targetAngle  = 80.0;
  }

  void begin() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    myServo.setPeriodHertz(50);
    myServo.attach(servoPin, 500, 2400);
    myServo.write((int)currentAngle);

    lastNormalHappyMove = millis();
  }

  void setState(RobotState state) {
    if (currentState == state) return;

    currentState = state;

    switch (currentState) {
      case DANGER_FIRE:
        targetAngle = 60.0;
        break;

      case DANGER_HUMID:
        targetAngle = 60.0;
        break;

      case WARNING_HOT:
        targetAngle = 60.0;
        break;

      case WARNING_COLD:
        targetAngle = 85.0;
        break;

      case WARNING_DARK:
      case SLEEP_MODE:
      case NORMAL_HAPPY:
        targetAngle = 90.0;
        if (currentState == NORMAL_HAPPY) {
          lastNormalHappyMove = millis();
        }
        break;

      case DANCE_MODE:
        targetAngle = 60.0;
        break;
    }
  }

  void update() {
    unsigned long now = millis();
    float step = 0.0;
    unsigned long interval = 20;

    if (isShaking) {
      if (now >= shakeEndTime) {
        isShaking = false;
        setState(currentState);
      } else {
        interval = 25;
        step     = 6.0;
        if (abs(currentAngle - targetAngle) < 1.0) {
          targetAngle = (targetAngle == 80.0) ? 100.0 : 80.0;
        }
      }
    }
    else {
      switch (currentState) {
        case DANGER_FIRE:
          interval = 10;
          step     = 6.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;

        case DANGER_HUMID:
          interval = 15;
          step     = 3.0;
          targetAngle = 60.0;
          break;

        case WARNING_HOT:
          interval = 50;
          step     = 1.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;

        case WARNING_COLD:
          interval = 30;
          step     = 10.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 85.0) ? 95.0 : 85.0;
          }
          break;

        case WARNING_DARK:
        case SLEEP_MODE:
          interval    = 20;
          step        = 2.0;
          targetAngle = 90.0;
          break;

        case NORMAL_HAPPY:
          interval = 30;
          step     = 1.0;
          if (now - lastNormalHappyMove >= 180000) {
            lastNormalHappyMove = now;
            int angles[] = {60, 75, 90, 105, 120};
            targetAngle  = angles[random(0, 5)];
          }
          break;

        case DANCE_MODE:
          interval = 12;
          step     = 5.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;
      }
    }

    if (now - lastUpdate >= interval) {
      lastUpdate = now;

      if (currentAngle < targetAngle) {
        currentAngle += step;
        if (currentAngle > targetAngle) currentAngle = targetAngle;
      } else if (currentAngle > targetAngle) {
        currentAngle -= step;
        if (currentAngle < targetAngle) currentAngle = targetAngle;
      }

      myServo.write((int)currentAngle);
    }
  }

  int getAngle() const { return (int)currentAngle; }
};

#endif
