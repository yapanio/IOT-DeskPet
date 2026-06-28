#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include "robot_state.h"
#include "sensors.h"
#include "robot_servo.h"
#include "actuators.h"
#include "emote.h"

// GPIO Pin Definitions
#define TOUCH_PIN 15
#define DHT_PIN 19
#define SERVO_PIN 14
#define LED_PIN 4
#define BUZZER_PIN 18

class Controller {
private:
    Sensors sensors;
    RobotServo servo;
    Actuators actuators;
    Emote emote;

    RobotState currentState = NORMAL_HAPPY;
    
    // Timers
    unsigned long lastTimeLightChecked = 0;
    unsigned long lastTimeBuzzerSounded = 0;
    
    // Debounce for touch sensor
    bool touchActive = false;
    unsigned long lastTouchTime = 0;

public:
    Controller() : 
        sensors(DHT_PIN), 
        servo(SERVO_PIN), 
        actuators(LED_PIN, BUZZER_PIN) 
    {}

    void begin() {
        Serial.begin(115200);
        while (!Serial && millis() < 3000); // Wait briefly for Serial debug
        Serial.println(F("Initializing Robot Controller (MVC)..."));

        // Set pins
        pinMode(TOUCH_PIN, INPUT);

        // Initialize sub-systems
        sensors.begin();
        servo.begin();
        actuators.begin();
        emote.begin(&sensors);

        // Set initial state
        servo.setState(currentState);
        actuators.setState(currentState);
        emote.setExpression(currentState);
        
        lastTimeLightChecked = millis();
        lastTimeBuzzerSounded = millis();
        
        Serial.println(F("Robot Controller fully initialized."));
    }

    void update() {
        // 1. Update sensor data (non-blocking)
        sensors.update();

        // 2. Evaluate state transitions
        evaluateState();

        // 3. Update non-blocking warning buzzer intervals
        updateBuzzerReminders();

        // 4. Read interactive touch input
        updateTouch();

        // 5. Run continuous updates for sub-systems
        servo.update();
        actuators.update();
        emote.update();
    }

private:
    void evaluateState() {
        float temp = sensors.getTemperature();
        float humid = sensors.getHumidity();
        float feel = sensors.getHeatIndex();
        float lux = sensors.getLightLux();
        unsigned long now = millis();

        RobotState nextState = NORMAL_HAPPY;

        // Reset the weak-light timer if we are not in the weak-light range
        if (lux >= 150.0 || lux < 50.0) {
            lastTimeLightChecked = now;
        }

        // Evaluate priority tree (highest priority first)
        if (temp > 42.0) {
            nextState = DANGER_FIRE;
        } 
        else if (humid > 85.0) {
            nextState = DANGER_HUMID;
        } 
        else if (temp > 30.0 || feel > 33.0) {
            nextState = WARNING_HOT;
        } 
        else if (temp < 18.0 && humid < 35.0) {
            nextState = WARNING_COLD;
        } 
        else if (lux < 50.0) {
            nextState = SLEEP_MODE;
        } 
        else if (lux < 150.0) {
            // Check if light has been weak (<150) for more than 10 mins (600,000 ms)
            if (now - lastTimeLightChecked > 600000) {
                nextState = WARNING_DARK;
            } else {
                // If 10 minutes hasn't elapsed, keep previous state if it was WARNING_DARK, 
                // otherwise fall back to NORMAL_HAPPY until the 10 mins pass.
                if (currentState == WARNING_DARK) {
                    nextState = WARNING_DARK;
                } else {
                    nextState = NORMAL_HAPPY;
                }
            }
        } 
        else {
            nextState = NORMAL_HAPPY;
        }

        // Apply state transition
        if (nextState != currentState) {
            Serial.print(F("State transition: "));
            Serial.print(stateToString(currentState));
            Serial.print(F(" -> "));
            Serial.println(stateToString(nextState));

            currentState = nextState;
            
            // Dispatch state change to sub-systems
            servo.setState(currentState);
            actuators.setState(currentState);
            emote.setExpression(currentState);
            
            // Set up buzzer timer so it triggers beeps immediately on warning entry
            if (currentState == WARNING_HOT) {
                lastTimeBuzzerSounded = now - 300000;
            } else if (currentState == WARNING_DARK) {
                lastTimeBuzzerSounded = now - 60000;
            } else {
                lastTimeBuzzerSounded = now;
            }
        }
    }

    void updateBuzzerReminders() {
        unsigned long now = millis();
        
        if (currentState == WARNING_HOT) {
            // Beep every 5 minutes (300,000 ms)
            if (now - lastTimeBuzzerSounded >= 300000) {
                lastTimeBuzzerSounded = now;
                actuators.triggerSingleBeep(1200, 150); // Short beep
            }
        } 
        else if (currentState == WARNING_DARK) {
            // Double beep every 1 minute (60,000 ms)
            if (now - lastTimeBuzzerSounded >= 60000) {
                lastTimeBuzzerSounded = now;
                actuators.triggerDoubleBeep(1500, 80, 80); // Double beep with 80ms gap
            }
        }
    }

    void updateTouch() {
        unsigned long now = millis();
        
        // Touch interaction debounce
        if (digitalRead(TOUCH_PIN) == HIGH) {
            if (!touchActive && (now - lastTouchTime > 1000)) {
                touchActive = true;
                lastTouchTime = now;
                
                Serial.println(F("Touch sensor triggered!"));
                
                // Only react if we are in normal state (not alarms or warnings)
                if (currentState == NORMAL_HAPPY) {
                    actuators.triggerDoubleBeep(2000, 80, 50); // Play happy beep
                    emote.triggerLaugh(); // Trigger RoboEyes laugh animation
                } else {
                    emote.triggerConfused(); // Play confused animation when warning is active
                }
            }
        } else {
            touchActive = false;
        }
    }

    const char* stateToString(RobotState state) {
        switch (state) {
            case DANGER_FIRE:  return "DANGER_FIRE";
            case DANGER_HUMID: return "DANGER_HUMID";
            case WARNING_HOT:  return "WARNING_HOT";
            case WARNING_COLD: return "WARNING_COLD";
            case WARNING_DARK: return "WARNING_DARK";
            case SLEEP_MODE:   return "SLEEP_MODE";
            case NORMAL_HAPPY: return "NORMAL_HAPPY";
            default:           return "UNKNOWN";
        }
    }
};

#endif
