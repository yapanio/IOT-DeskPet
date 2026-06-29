#ifndef ROBOT_SERVO_H
#define ROBOT_SERVO_H

#include "robot_state.h"
#include <ESP32Servo.h>

class RobotServo {
private:
  Servo myServo;
  int servoPin;
  float currentAngle = 90.0;
  float targetAngle = 90.0;
  unsigned long lastUpdate = 0;

  RobotState currentState = NORMAL_HAPPY;
  unsigned long lastNormalHappyMove = 0;

public:
  RobotServo(int pin) : servoPin(pin) {}

  void begin() {
    // ESP32Servo setup
    // Allocate timers for PWM
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    myServo.setPeriodHertz(50); // Standard 50hz servo
    myServo.attach(servoPin, 500,
                   2400); // Attach pin with min/max pulse width in microseconds
    myServo.write((int)currentAngle);
    lastNormalHappyMove = millis();
  }

  void setState(RobotState state) {
    if (currentState != state) {
      currentState = state;
      // Reset state-dependent variables
      switch (currentState) {
      case DANGER_FIRE:
        targetAngle = 60.0; // Start sweeping between 60 and 120
        break;
      case DANGER_HUMID:
        targetAngle =
            60.0; // Rotate completely away from humidity source (60 deg)
        break;
      case WARNING_HOT:
        targetAngle = 60.0; // Start slow sweep (60 to 120)
        break;
      case WARNING_COLD:
        targetAngle = 85.0; // Start shivering (85 to 95)
        break;
      case WARNING_DARK:
        targetAngle = 90.0; // Keep centered (cannot look up)
        break;
      case SLEEP_MODE:
        targetAngle = 90.0; // Return to center
        break;
      case NORMAL_HAPPY:
        targetAngle = 90.0;
        lastNormalHappyMove = millis();
        break;
      case DANCE_MODE:
        targetAngle = 60.0; // Start sweeping quickly between 60 and 120
        break;
      }
    }
  }

  void update() {
    unsigned long now = millis();
    float step = 0.0;
    unsigned long interval = 20;

    switch (currentState) {
    case DANGER_FIRE:
      interval = 10; // Fast updates
      step = 6.0;    // Rapid movements
      if (abs(currentAngle - targetAngle) < 1.0) {
        targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
      }
      break;

    case DANGER_HUMID:
      interval = 15;
      step = 3.0;
      targetAngle = 60.0;
      break;

    case WARNING_HOT:
      interval = 50; // Slow updates
      step = 1.0;    // Small step
      if (abs(currentAngle - targetAngle) < 1.0) {
        targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
      }
      break;

    case WARNING_COLD:
      // Shiver: quick oscillation around center (90) by +/-5 degrees (85 to 95)
      interval = 30;
      step = 10.0; // Jump quickly between limits
      if (abs(currentAngle - targetAngle) < 1.0) {
        targetAngle = (targetAngle == 85.0) ? 95.0 : 85.0;
      }
      break;

    case WARNING_DARK:
      interval = 20;
      step = 2.0;
      targetAngle = 90.0;
      break;

    case SLEEP_MODE:
      interval = 20;
      step = 1.5;
      targetAngle = 90.0;
      break;

    case NORMAL_HAPPY:
      interval = 30;
      step = 1.0;
      // Move head randomly 15 degrees left/right of center every 3 minutes
      // (180000 ms)
      if (now - lastNormalHappyMove >= 180000) {
        lastNormalHappyMove = now;
        int angles[] = {60, 75, 90, 105, 120};
        targetAngle = angles[random(0, 5)];
      }
      break;

    case DANCE_MODE:
      interval = 12; // Fast updates
      step = 5.0;    // Rapid movements
      if (abs(currentAngle - targetAngle) < 1.0) {
        targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
      }
      break;
    }

    // Smooth interpolation of head movement
    if (now - lastUpdate >= interval) {
      lastUpdate = now;

      if (currentAngle < targetAngle) {
        currentAngle += step;
        if (currentAngle > targetAngle) {
          currentAngle = targetAngle;
        }
      } else if (currentAngle > targetAngle) {
        currentAngle -= step;
        if (currentAngle < targetAngle) {
          currentAngle = targetAngle;
        }
      }

      myServo.write((int)currentAngle);
    }
  }

  int getAngle() const { return (int)currentAngle; }
};

#endif
