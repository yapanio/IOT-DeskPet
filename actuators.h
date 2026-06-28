#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "robot_state.h"
#include <Adafruit_NeoPixel.h>

#define LED_COUNT 12

class Actuators {
private:
  Adafruit_NeoPixel strip;
  int buzzerPin;
  RobotState currentState = NORMAL_HAPPY;

  unsigned long lastLedUpdate = 0;

  // Buzzer state machine for one-shot sequences
  unsigned long buzzerSeqStart = 0;
  int buzzerSeqStep = 0; // 0: idle, 1: beep1 active, 2: gap, 3: beep2 active
  int buzzerFreq = 0;
  int buzzerDur = 0;
  int buzzerGap = 0;

  // Buzzer state for continuous alarm siren
  unsigned long lastAlarmToggle = 0;
  bool alarmToggleState = false;

public:
  Actuators(int ledPin, int buzzerPin) 
      : strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800), buzzerPin(buzzerPin) {}

  void begin() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    pinMode(buzzerPin, OUTPUT);
    noTone(buzzerPin);
  }

  void setState(RobotState state) {
    if (currentState != state) {
      currentState = state;

      // Turn off buzzer immediately and clear pending beep sequences
      noTone(buzzerPin);
      buzzerSeqStep = 0;
      lastAlarmToggle = 0;
      alarmToggleState = false;
    }
  }

  // Trigger a single beep
  void triggerSingleBeep(int frequency, int duration) {
    // Only trigger if no alarm state is active and buzzer is idle
    if (currentState != DANGER_FIRE && currentState != DANGER_HUMID &&
        buzzerSeqStep == 0) {
      buzzerFreq = frequency;
      buzzerDur = duration;
      buzzerGap = 0;
      buzzerSeqStart = millis();
      buzzerSeqStep = 1;
    }
  }

  // Trigger a double beep
  void triggerDoubleBeep(int frequency, int duration, int gap) {
    if (currentState != DANGER_FIRE && currentState != DANGER_HUMID &&
        buzzerSeqStep == 0) {
      buzzerFreq = frequency;
      buzzerDur = duration;
      buzzerGap = gap;
      buzzerSeqStart = millis();
      buzzerSeqStep = 1;
    }
  }

  void update() {
    updateLEDs();
    updateBuzzer();
  }

private:
  void updateLEDs() {
    unsigned long now = millis();
    if (now - lastLedUpdate < 30)
      return; // rate limit to 30ms (approx 33fps)
    lastLedUpdate = now;

    switch (currentState) {
    case DANGER_FIRE: {
      // Rapid red strobe (every 100ms)
      bool on = (now / 100) % 2;
      uint32_t color = on ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case DANGER_HUMID: {
      // Solid red (100% brightness)
      uint32_t color = strip.Color(255, 0, 0);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case WARNING_HOT: {
      // Solid orange (70% brightness: Red 178, Green 115, Blue 0)
      uint32_t color = strip.Color(178, 115, 0);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case WARNING_COLD: {
      // Solid light cyan/white (70% brightness: Red 0, Green 178, Blue 178)
      uint32_t color = strip.Color(0, 178, 178);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case WARNING_DARK: {
      // Slow flashing yellow (every 500ms)
      bool on = (now / 500) % 2;
      uint32_t color = on ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case SLEEP_MODE: {
      // Dim purple (10% brightness: Red 25, Green 0, Blue 25)
      uint32_t color = strip.Color(25, 0, 25);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    case NORMAL_HAPPY: {
      // Green breathing (3000ms period)
      float factor = (sin(now * 2.0 * 3.14159265 / 3000.0) + 1.0) / 2.0;
      int greenValue = factor * 255;
      uint32_t color = strip.Color(0, greenValue, 0);
      for (int i = 0; i < LED_COUNT; i++)
        strip.setPixelColor(i, color);
      strip.show();
      break;
    }
    }
  }

  void updateBuzzer() {
    unsigned long now = millis();

    if (currentState == DANGER_FIRE) {
      // Rapid alarm siren: alternating frequency every 120ms
      if (now - lastAlarmToggle >= 120) {
        lastAlarmToggle = now;
        alarmToggleState = !alarmToggleState;
        tone(buzzerPin, alarmToggleState ? 2500 : 1800);
      }
    } else if (currentState == DANGER_HUMID) {
      // Continuous warning tone at 1000Hz
      tone(buzzerPin, 1000);
    } else {
      // Sequence beep states (non-blocking)
      if (buzzerSeqStep == 1) {
        if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
          tone(buzzerPin, buzzerFreq);
        } else {
          noTone(buzzerPin);
          if (buzzerGap > 0) {
            buzzerSeqStep = 2;
          } else {
            buzzerSeqStep = 0;
          }
          buzzerSeqStart = now;
        }
      } else if (buzzerSeqStep == 2) {
        noTone(buzzerPin);
        if (now - buzzerSeqStart >= (unsigned long)buzzerGap) {
          buzzerSeqStep = 3;
          buzzerSeqStart = now;
        }
      } else if (buzzerSeqStep == 3) {
        if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
          tone(buzzerPin, buzzerFreq);
        } else {
          noTone(buzzerPin);
          buzzerSeqStep = 0;
        }
      }
    }
  }
};

#endif
