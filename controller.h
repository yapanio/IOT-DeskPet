#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include "robot_state.h"
#include "sensors.h"
#include "robot_servo.h"
#include "actuators.h"
#include "emote.h"
#include "blynk_service.h"

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
    BlynkService blynk;

    RobotState currentState = NORMAL_HAPPY;
    
    // Timers
    unsigned long lastTimeLightChecked = 0;
    unsigned long lastTimeBuzzerSounded = 0;
    
    // Debounce for touch sensor
    bool touchActive = false;
    unsigned long lastTouchTime = 0;
    bool mockTouchTriggered = false;

    // Touch state machine variables
    bool lastTouchState = false;
    unsigned long touchStartTime = 0;
    unsigned long lastTapTime = 0;
    int tapCount = 0;
    bool longPressDetected = false;

    // Dance Mode control variables
    bool isDancing = false;
    unsigned long danceEndTime = 0;
    RobotState preDanceState = NORMAL_HAPPY;

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
        blynk.begin();

        // Set initial state
        servo.setState(currentState);
        actuators.setState(currentState);
        emote.setExpression(currentState);
        
        lastTimeLightChecked = millis();
        lastTimeBuzzerSounded = millis();
        
        Serial.println(F("Robot Controller fully initialized."));
    }

    void update() {
        // Read serial commands for simulation mode
        handleSerialCommands();

        // 1. Update sensor data (non-blocking)
        sensors.update();

        // Check if Dance Mode timer has ended or song completed
        if (isDancing) {
            unsigned long now = millis();
            bool songFinished = !actuators.isSongPlaying();
            if (now >= danceEndTime || songFinished) {
                Serial.println(F("[Dance Mode] Dance completed. Returning to normal."));
                isDancing = false;
                actuators.stopSong();
                currentState = preDanceState;
                servo.setState(currentState);
                actuators.setState(currentState);
                emote.setExpression(currentState);
            }
        }

        // 2. Evaluate state transitions (only if not dancing)
        if (!isDancing) {
            evaluateState();
        }

        // 3. Update non-blocking warning buzzer intervals (only if not dancing)
        if (!isDancing) {
            updateBuzzerReminders();
        }

        // 4. Read interactive touch input
        updateTouch();

        // 5. Run continuous updates for sub-systems
        servo.update();
        actuators.update();
        emote.update();

        // 6. Update Blynk
        blynk.update(
            sensors.getTemperature(),
            sensors.getHumidity(),
            sensors.getHeatIndex(),
            sensors.getLightLux(),
            stateToString(currentState)
        );
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
        else if (temp > 33.0 || feel > 35.0) {
            nextState = WARNING_HOT;
        } 
        else if (temp < 18.0 && humid < 36.0) {
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
                // Auto play Despacito (Song 2) once on hot warning
                actuators.playSong(2);
            } else if (currentState == WARNING_COLD) {
                lastTimeBuzzerSounded = now;
                // Auto play Jingle Bells (Song 3) once on cold warning
                actuators.playSong(3);
            } else if (currentState == WARNING_DARK) {
                lastTimeBuzzerSounded = now - 60000;
                actuators.stopSong();
            } else {
                lastTimeBuzzerSounded = now;
                actuators.stopSong();
            }
        }
    }

    void updateBuzzerReminders() {
        unsigned long now = millis();
        
        // If a song is currently playing, do not play warning beeps
        if (actuators.isSongPlaying()) {
            return;
        }
        
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

    void handleSingleTap() {
        Serial.println(F("Handling Single Tap."));
        if (currentState == NORMAL_HAPPY) {
            actuators.triggerDoubleBeep(2000, 80, 50); // Play happy beep
            emote.triggerLaugh(); // Trigger RoboEyes laugh animation
        } else {
            emote.triggerConfused(); // Play confused animation when warning/danger is active
        }
    }

    void handleDoubleTap() {
        Serial.println(F("Handling Double Tap (Wink)."));
        // Cheer beep
        actuators.triggerDoubleBeep(2500, 60, 60);
        // Play wink animation
        emote.triggerWink();
    }

    void triggerDanceMode(int songId = 1) {
        Serial.print(F("Triggering DANCE MODE with Song ID: "));
        Serial.println(songId);
        isDancing = true;
        danceEndTime = millis() + 60000; // Max dance for 60 seconds (or until song finishes)
        preDanceState = currentState;   // Save previous state to restore later
        currentState = DANCE_MODE;
        
        // Dispatch state change to sub-systems
        servo.setState(currentState);
        actuators.setState(currentState);
        emote.setExpression(currentState);
        
        // Start song playback
        actuators.playSong(songId);
    }

    void stopDanceMode() {
        if (isDancing) {
            Serial.println(F("[Dance Mode] Stopped."));
            isDancing = false;
            actuators.stopSong();
            currentState = preDanceState;
            servo.setState(currentState);
            actuators.setState(currentState);
            emote.setExpression(currentState);
        }
    }

    void playSongBlynk(int songId) {
        if (songId >= 1 && songId <= 3) {
            triggerDanceMode(songId);
        } else {
            stopDanceMode();
        }
    }

    void updateTouch() {
        if (isDancing) {
            mockTouchTriggered = false;
            return;
        }

        unsigned long now = millis();
        bool isTouched = (digitalRead(TOUCH_PIN) == HIGH) || mockTouchTriggered;

        // Detect touch press (rising edge)
        if (isTouched && !lastTouchState) {
            touchStartTime = now;
            longPressDetected = false;
        }

        // Detect touch release (falling edge)
        if (!isTouched && lastTouchState) {
            unsigned long pressDuration = now - touchStartTime;

            // Only count as tap if it wasn't already triggered as a long press
            if (!longPressDetected && pressDuration > 50 && pressDuration < 600) {
                tapCount++;
                lastTapTime = now;
            }
            mockTouchTriggered = false; // Reset mock trigger
        }

        // Detect long press while holding (does not wait for release)
        if (isTouched && !longPressDetected) {
            unsigned long pressDuration = now - touchStartTime;
            if (pressDuration >= 3000) { // Held for 3 seconds
                longPressDetected = true;
                tapCount = 0; // Clear pending taps
                triggerDanceMode(1); // Play Super Mario (Song 1)
            }
        }

        // Evaluate tap count after a short timeout (400ms after last tap)
        if (tapCount > 0 && (now - lastTapTime > 400)) {
            if (tapCount == 1) {
                handleSingleTap();
            } else if (tapCount >= 2) {
                handleDoubleTap();
            }
            tapCount = 0; // Reset
        }

        lastTouchState = isTouched;
    }

    void handleSerialCommands() {
        if (Serial.available() > 0) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            
            if (cmd.length() == 0) return;
            
            Serial.print(F("Received Command: "));
            Serial.println(cmd);
            
            if (cmd.equalsIgnoreCase("test 1") || cmd.equalsIgnoreCase("danger_fire")) {
                sensors.setMock(true, 45.0, 50.0, 350.0);
                Serial.println(F("[TEST MODE] Enabled DANGER_FIRE simulation. (Temp = 45C, Humid = 50%, Lux = 350)"));
            }
            else if (cmd.equalsIgnoreCase("test 2") || cmd.equalsIgnoreCase("danger_humid")) {
                sensors.setMock(true, 25.0, 90.0, 350.0);
                Serial.println(F("[TEST MODE] Enabled DANGER_HUMID simulation. (Temp = 25C, Humid = 90%, Lux = 350)"));
            }
            else if (cmd.equalsIgnoreCase("test 3") || cmd.equalsIgnoreCase("warning_hot")) {
                sensors.setMock(true, 35.0, 50.0, 350.0);
                Serial.println(F("[TEST MODE] Enabled WARNING_HOT simulation. (Temp = 35C, Humid = 50%, Lux = 350)"));
            }
            else if (cmd.equalsIgnoreCase("test 4") || cmd.equalsIgnoreCase("warning_cold")) {
                sensors.setMock(true, 15.0, 30.0, 350.0);
                Serial.println(F("[TEST MODE] Enabled WARNING_COLD simulation. (Temp = 15C, Humid = 30%, Lux = 350)"));
            }
            else if (cmd.equalsIgnoreCase("test 5") || cmd.equalsIgnoreCase("sleep_mode")) {
                sensors.setMock(true, 25.0, 55.0, 20.0);
                Serial.println(F("[TEST MODE] Enabled SLEEP_MODE simulation. (Temp = 25C, Humid = 55%, Lux = 20)"));
            }
            else if (cmd.equalsIgnoreCase("test 6") || cmd.equalsIgnoreCase("warning_dark")) {
                sensors.setMock(true, 25.0, 55.0, 100.0);
                lastTimeLightChecked = millis() - 605000; // Bypass the 10 min threshold
                Serial.println(F("[TEST MODE] Enabled WARNING_DARK simulation. (Temp = 25C, Humid = 55%, Lux = 100). Bypassed 10-min timer."));
            }
            else if (cmd.equalsIgnoreCase("test 7") || cmd.equalsIgnoreCase("touch")) {
                mockTouchTriggered = true;
                Serial.println(F("[TEST MODE] Triggered mock touch interaction."));
            }
            else if (cmd.equalsIgnoreCase("test 8") || cmd.equalsIgnoreCase("double_tap")) {
                handleDoubleTap();
            }
            else if (cmd.equalsIgnoreCase("test 9") || cmd.equalsIgnoreCase("long_press") || cmd.equalsIgnoreCase("dance")) {
                triggerDanceMode();
            }
            else if (cmd.equalsIgnoreCase("normal") || cmd.equalsIgnoreCase("exit")) {
                sensors.setMock(false, 0, 0, 0);
                Serial.println(F("[TEST MODE] Disabled simulation. Resuming physical sensors reading."));
            }
            else {
                Serial.println(F("Unknown command! Available commands:"));
                Serial.println(F("  - test 1 / danger_fire"));
                Serial.println(F("  - test 2 / danger_humid"));
                Serial.println(F("  - test 3 / warning_hot"));
                Serial.println(F("  - test 4 / warning_cold"));
                Serial.println(F("  - test 5 / sleep_mode"));
                Serial.println(F("  - test 6 / warning_dark"));
                Serial.println(F("  - test 7 / touch"));
                Serial.println(F("  - test 8 / double_tap"));
                Serial.println(F("  - test 9 / long_press / dance"));
                Serial.println(F("  - normal / exit"));
            }
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
            case DANCE_MODE:   return "DANCE_MODE";
            default:           return "UNKNOWN";
        }
    }
};

#endif
