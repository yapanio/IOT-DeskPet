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
    // ==========================================
    // Subsystem Instances
    // ==========================================
    Sensors sensors;
    RobotServo servo;
    Actuators actuators;
    Emote emote;
    BlynkService blynk;

    // ==========================================
    // Core Controller State
    // ==========================================
    RobotState currentState = NORMAL_HAPPY;
    bool testModeActive = false;    // Toggled by long pressing physical touch sensor
    bool alarmMuted = false;        // Silences active warning buzzers

    // ==========================================
    // Timers & Intervals
    // ==========================================
    unsigned long lastTimeLightChecked = 0;  // Tracks lux threshold duration
    unsigned long lastTimeBuzzerSounded = 0; // Tracks interval between warning beeps

    // ==========================================
    // Touch Input & Gesture Variables
    // ==========================================
    bool blynkTouchState = false;   // Virtual touch state from Blynk app
    bool lastTouchState = false;    // For edge detection
    unsigned long touchStartTime = 0;
    unsigned long lastTapTime = 0;
    int tapCount = 0;
    bool longPressDetected = false;

    // ==========================================
    // Dance Mode Control Variables
    // ==========================================
    bool isDancing = false;
    unsigned long danceEndTime = 0;
    RobotState preDanceState = NORMAL_HAPPY; // Remembers state before dancing

    // ==========================================
    // UI Screen State
    // ==========================================
    bool showParamScreen = false;
    bool returnToParamScreenAfterDance = false;

public:
    Controller() : 
        sensors(DHT_PIN), 
        servo(SERVO_PIN), 
        actuators(LED_PIN, BUZZER_PIN) 
    {}

    /**
     * @brief Initializes serial and all integrated sub-systems.
     */
    void begin() {
        Serial.begin(115200);
        while (!Serial && millis() < 3000); // Wait briefly for Serial debug
        Serial.println(F("Initializing Robot Controller (MVC)..."));

        // Setup pins
        pinMode(TOUCH_PIN, INPUT);

        // Initialize sub-systems
        sensors.begin();
        servo.begin();
        actuators.begin();
        emote.begin(&sensors);
        blynk.begin();

        // Apply initial state
        applyStateToSubsystems();
        
        lastTimeLightChecked = millis();
        lastTimeBuzzerSounded = millis();
        
        Serial.println(F("Robot Controller fully initialized."));
    }

    /**
     * @brief The main run loop, to be called inside Arduino's loop().
     */
    void update() {
        // 1. Update non-blocking sensor readings
        sensors.update();

        // 2. Check if Dance Mode timer has elapsed or the song has completed
        if (isDancing) {
            unsigned long now = millis();
            bool songFinished = !actuators.isSongPlaying();
            if (now >= danceEndTime || songFinished) {
                Serial.println(F("[Dance Mode] Dance completed. Returning to normal."));
                stopDanceMode();
            }
        }

        // 3. Evaluate environmental state transitions and buzzer triggers
        if (!isDancing) {
            evaluateState();
            updateBuzzerReminders();
        }

        // 4. Update and parse physical/virtual touch gestures
        updateTouch();

        // 5. Run continuous updates for sub-systems
        servo.update();
        actuators.update();
        emote.update();

        // 6. Push telemetry to Blynk IoT cloud
        blynk.update(
            sensors.getTemperature(),
            sensors.getHumidity(),
            sensors.getHeatIndex(),
            sensors.getLightLux(),
            stateToString(currentState)
        );
    }

    /**
     * @brief Plays a song triggered via Blynk.
     */
    void playSongBlynk(int songId) {
        if (songId >= 1 && songId <= 3) {
            triggerDanceMode(songId);
        } else {
            stopDanceMode();
        }
    }

    /**
     * @brief Updates the virtual touch sensor state received from Blynk.
     */
    void setBlynkTouch(bool pressed) {
        blynkTouchState = pressed;
        Serial.print(F("[Blynk] Virtual touch state: "));
        Serial.println(blynkTouchState ? F("PRESSED") : F("RELEASED"));
    }

private:
    // ==========================================
    // State & Transition Management
    // ==========================================

    /**
     * @brief Updates sub-systems to match the current state.
     */
    void applyStateToSubsystems() {
        servo.setState(currentState);
        actuators.setState(currentState);
        emote.setExpression(currentState);
    }

    /**
     * @brief Centralized state transition handler.
     */
    void changeState(RobotState newState, bool resetAlarm = true) {
        if (currentState != newState) {
            Serial.print(F("State transition: "));
            Serial.print(stateToString(currentState));
            Serial.print(F(" -> "));
            Serial.println(stateToString(newState));

            currentState = newState;
            if (resetAlarm) {
                alarmMuted = false;
            }
            
            applyStateToSubsystems();
        }
    }

    /**
     * @brief Analyzes environmental sensor values to decide target RobotState.
     */
    void evaluateState() {
        float temp = sensors.getTemperature();
        float humid = sensors.getHumidity();
        float feel = sensors.getHeatIndex();
        float lux = sensors.getLightLux();
        unsigned long now = millis();

        RobotState nextState = NORMAL_HAPPY;

        // Reset the weak-light timer if light level is out of the warning range
        if (lux >= 150.0 || lux < 50.0) {
            lastTimeLightChecked = now;
        }

        // State Priority Tree
        if (temp > 42.0) {
            nextState = DANGER_FIRE;
        } 
        else if (humid > 85.0) {
            nextState = DANGER_HUMID;
        } 
        else if (temp > 35.0 || feel > 38.0) {
            nextState = WARNING_HOT;
        } 
        else if (temp < 18.0 && humid < 36.0) {
            nextState = WARNING_COLD;
        } 
        else if (lux < 50.0) {
            nextState = SLEEP_MODE;
        } 
        else if (lux < 150.0) {
            // Check if light is weak for more than 10 continuous minutes
            if (now - lastTimeLightChecked > 600000) {
                nextState = WARNING_DARK;
            } else {
                nextState = (currentState == WARNING_DARK) ? WARNING_DARK : NORMAL_HAPPY;
            }
        } 
        else {
            nextState = NORMAL_HAPPY;
        }

        // Apply state changes and configure appropriate initial action triggers
        if (nextState != currentState) {
            changeState(nextState, true);
            
            if (currentState == WARNING_HOT) {
                lastTimeBuzzerSounded = now - 300000;
                actuators.playSong(2); // Auto play Despacito (Song 2) once
            } else if (currentState == WARNING_COLD) {
                lastTimeBuzzerSounded = now;
                actuators.playSong(3); // Auto play Jingle Bells (Song 3) once
            } else if (currentState == WARNING_DARK) {
                lastTimeBuzzerSounded = now - 60000;
                actuators.stopSong();
            } else {
                lastTimeBuzzerSounded = now;
                actuators.stopSong();
            }
        }
    }

    /**
     * @brief Plays repetitive alarm beeps when in warning states (unless muted).
     */
    void updateBuzzerReminders() {
        if (alarmMuted || actuators.isSongPlaying()) {
            return;
        }

        unsigned long now = millis();
        
        if (currentState == WARNING_HOT) {
            // Beep every 5 minutes (300,000 ms)
            if (now - lastTimeBuzzerSounded >= 300000) {
                lastTimeBuzzerSounded = now;
                actuators.triggerSingleBeep(1200, 150);
            }
        } 
        else if (currentState == WARNING_DARK) {
            // Double beep every 1 minute (60,000 ms)
            if (now - lastTimeBuzzerSounded >= 60000) {
                lastTimeBuzzerSounded = now;
                actuators.triggerDoubleBeep(1500, 80, 80);
            }
        }
    }

    // ==========================================
    // Dance Mode Controls
    // ==========================================

    /**
     * @brief Triggers Dance Mode and locks the state.
     */
    void triggerDanceMode(int songId = 1) {
        Serial.print(F("Triggering DANCE MODE with Song ID: "));
        Serial.println(songId);
        isDancing = true;
        danceEndTime = millis() + 10000; // Limit dance to 10 seconds max
        preDanceState = currentState;
        
        changeState(DANCE_MODE, false);
        actuators.playSong(songId);
    }

    /**
     * @brief Stops Dance Mode and restores the previous state.
     */
    void stopDanceMode() {
        if (isDancing) {
            Serial.println(F("[Dance Mode] Stopped."));
            isDancing = false;
            actuators.stopSong();
            
            changeState(preDanceState, false);

            if (returnToParamScreenAfterDance) {
                showParamScreen = true;
                emote.setShowParamScreen(true);
                returnToParamScreenAfterDance = false;
            }
        }
    }

    // ==========================================
    // Gesture & Touch Event Handlers
    // ==========================================

    /**
     * @brief Mutes sirens or cycles screen mode on physical touch single tap.
     */
    void handleSingleTap() {
        // If in physical test/simulation mode, tap to cycle states
        if (testModeActive) {
            Serial.println(F("[Test Mode] Single tap detected. Cycling state."));
            switch (currentState) {
                case NORMAL_HAPPY:
                    sensors.setMock(true, 45.0, 50.0, 350.0); // -> DANGER_FIRE
                    Serial.println(F("[Test Mode] Mocking DANGER_FIRE."));
                    break;
                case DANGER_FIRE:
                    sensors.setMock(true, 25.0, 90.0, 350.0); // -> DANGER_HUMID
                    Serial.println(F("[Test Mode] Mocking DANGER_HUMID."));
                    break;
                case DANGER_HUMID:
                    sensors.setMock(true, 35.0, 50.0, 350.0); // -> WARNING_HOT
                    Serial.println(F("[Test Mode] Mocking WARNING_HOT."));
                    break;
                case WARNING_HOT:
                    sensors.setMock(true, 15.0, 30.0, 350.0); // -> WARNING_COLD
                    Serial.println(F("[Test Mode] Mocking WARNING_COLD."));
                    break;
                case WARNING_COLD:
                    sensors.setMock(true, 25.0, 55.0, 20.0);  // -> SLEEP_MODE
                    Serial.println(F("[Test Mode] Mocking SLEEP_MODE."));
                    break;
                case SLEEP_MODE:
                    sensors.setMock(true, 25.0, 55.0, 100.0); // -> WARNING_DARK
                    lastTimeLightChecked = millis() - 605000;
                    Serial.println(F("[Test Mode] Mocking WARNING_DARK."));
                    break;
                case WARNING_DARK:
                default:
                    sensors.setMock(false, 0, 0, 0); // Disable mock and return to physical sensors
                    Serial.println(F("[Test Mode] Disabling Mock (Returning to NORMAL_HAPPY)."));
                    break;
            }
            alarmMuted = false;
            actuators.setMuted(false);
            return;
        }

        // Normal Mode single tap handling
        Serial.println(F("Handling Single Tap in Normal Mode."));
        bool alarmSounding = !alarmMuted && (currentState == DANGER_FIRE || currentState == DANGER_HUMID || actuators.isSongPlaying());

        if (alarmSounding) {
            Serial.println(F("Alarm is active and sounding. Silencing it (Muting)."));
            alarmMuted = true;
            actuators.setMuted(true);
        } else {
            showParamScreen = !showParamScreen;
            emote.setShowParamScreen(showParamScreen);
            Serial.print(F("Toggling Parameter Screen Mode. Active: "));
            Serial.println(showParamScreen);
            
            actuators.triggerSingleBeep(1500, 100);
        }
    }

    /**
     * @brief Performs a wink & shake head gesture on double tap.
     */
    void handleDoubleTap() {
        if (showParamScreen) {
            Serial.println(F("Double Tap on parameter screen. Ignored."));
            return;
        }
        Serial.println(F("Handling Double Tap (Wink & Shake Head)."));
        actuators.triggerDoubleBeep(2500, 60, 60);
        emote.triggerWink();
        servo.triggerGentleShake();
    }

    /**
     * @brief Enters dance mode on triple tap.
     */
    void handleTripleTap() {
        Serial.println(F("Handling Triple Tap (Dance Mode)."));
        if (showParamScreen) {
            returnToParamScreenAfterDance = true;
            showParamScreen = false;
            emote.setShowParamScreen(false);
        } else {
            returnToParamScreenAfterDance = false;
        }
        triggerDanceMode(1); // Play Super Mario theme
        danceEndTime = millis() + 5000; // Limit this specific dance gesture to 5 seconds
    }

    /**
     * @brief Detects touch gestures (tap, double-tap, triple-tap, long-press).
     */
    void updateTouch() {
        if (isDancing) {
            return;
        }

        unsigned long now = millis();
        bool isTouched = (digitalRead(TOUCH_PIN) == HIGH) || blynkTouchState;

        // Detect touch press (rising edge)
        if (isTouched && !lastTouchState) {
            touchStartTime = now;
            longPressDetected = false;
        }

        // Detect touch release (falling edge)
        if (!isTouched && lastTouchState) {
            unsigned long pressDuration = now - touchStartTime;

            // Register tap if released within normal duration and not part of a long press
            if (!longPressDetected && pressDuration > 50 && pressDuration < 600) {
                tapCount++;
                lastTapTime = now;
            }
        }

        // Detect long press while holding (does not wait for release)
        if (isTouched && !longPressDetected) {
            unsigned long pressDuration = now - touchStartTime;
            if (pressDuration >= 2000) { // Held for 2 seconds
                longPressDetected = true;
                tapCount = 0; // Cancel pending tap detections
                
                testModeActive = !testModeActive;
                Serial.print(F("Toggling Test Mode. Active: "));
                Serial.println(testModeActive);
                
                if (testModeActive) {
                    alarmMuted = false;
                    actuators.setMuted(false);
                    actuators.triggerDoubleBeep(2000, 100, 100);
                } else {
                    sensors.setMock(false, 0, 0, 0);
                    alarmMuted = false;
                    actuators.setMuted(false);
                    actuators.triggerSingleBeep(1000, 200);
                }
            }
        }

        // Dispatch tap callbacks after a short timeout (400ms gap after last touch release)
        if (tapCount > 0 && (now - lastTapTime > 400)) {
            if (tapCount == 1) {
                handleSingleTap();
            } else if (tapCount == 2) {
                handleDoubleTap();
            } else if (tapCount >= 3) {
                handleTripleTap();
            }
            tapCount = 0;
        }

        lastTouchState = isTouched;
    }

    // ==========================================
    // Telemetry Formatting Helpers
    // ==========================================
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
