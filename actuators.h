#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "robot_state.h"
#include "songs.h"
#include <Adafruit_NeoPixel.h>
#include <esp_arduino_version.h>

#define LED_COUNT 12

class Actuators {
private:
  Adafruit_NeoPixel strip;
  int buzzerPin;
  RobotState currentState = NORMAL_HAPPY;

  unsigned long lastLedUpdate = 0;

  // Non-blocking song playing variables
  int currentSong = 0; // 0: Idle, 1: Mario, 2: Despacito, 3: Jingle Bells
  int currentNoteIndex = 0;
  unsigned long nextNoteTime = 0;
  bool isSilentGap = false;

  // Buzzer state machine for one-shot sequences
  unsigned long buzzerSeqStart = 0;
  int buzzerSeqStep = 0; // 0: idle, 1: beep1 active, 2: gap, 3: beep2 active
  int buzzerFreq = 0;
  int buzzerDur = 0;
  int buzzerGap = 0;

  // Buzzer state for continuous alarm siren
  unsigned long lastAlarmToggle = 0;
  bool alarmToggleState = false;

  // Volume duty cycle: 5 out of 255 (approx 2% duty cycle)
  const int buzzerVolume = 5; 

  // Cross-compatible tone generator using LEDC
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  void playTone(int frequency) {
    if (frequency <= 0) {
      stopTone();
      return;
    }
    ledcAttach(buzzerPin, frequency, 8);
    ledcWrite(buzzerPin, buzzerVolume);
  }

  void stopTone() {
    ledcWrite(buzzerPin, 0);
    ledcDetach(buzzerPin);
  }
#else
  void playTone(int frequency) {
    if (frequency <= 0) {
      stopTone();
      return;
    }
    ledcSetup(1, frequency, 8);
    ledcAttachPin(buzzerPin, 1);
    ledcWrite(1, buzzerVolume);
  }

  void stopTone() {
    ledcWrite(1, 0);
  }
#endif

public:
  Actuators(int ledPin, int buzzerPin) 
      : strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800), buzzerPin(buzzerPin) {}

  void begin() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    pinMode(buzzerPin, OUTPUT);
    stopTone();
  }

  void setState(RobotState state) {
    if (currentState != state) {
      currentState = state;

      // Turn off buzzer immediately and clear pending beep sequences
      stopTone();
      stopSong();
      buzzerSeqStep = 0;
      lastAlarmToggle = 0;
      alarmToggleState = false;
    }
  }

  void playSong(int songId) {
    if (songId >= 1 && songId <= 3) {
      currentSong = songId;
      currentNoteIndex = 0;
      nextNoteTime = millis();
      isSilentGap = false;
      stopTone();
    } else {
      stopSong();
    }
  }

  void stopSong() {
    currentSong = 0;
    stopTone();
  }

  bool isSongPlaying() const {
    return currentSong != 0;
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
  uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

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
    case DANCE_MODE: {
      // Rainbow cycle: shifting colors across pixels over time
      int shift = (now / 5) % 256;
      for (int i = 0; i < LED_COUNT; i++) {
        byte colorPos = (i * 256 / LED_COUNT + shift) % 256;
        strip.setPixelColor(i, Wheel(colorPos));
      }
      strip.show();
      break;
    }
    }
  }

  void updateBuzzer() {
    unsigned long now = millis();

    // Danger states have absolute priority and will override any song
    if (currentState == DANGER_FIRE) {
      if (currentSong != 0) stopSong();
      // Rapid alarm siren: alternating frequency every 120ms
      if (now - lastAlarmToggle >= 120) {
        lastAlarmToggle = now;
        alarmToggleState = !alarmToggleState;
        playTone(alarmToggleState ? 2500 : 1800);
      }
      return;
    } 
    
    if (currentState == DANGER_HUMID) {
      if (currentSong != 0) stopSong();
      // Continuous warning tone at 1000Hz
      playTone(1000);
      return;
    }

    // If a song is playing, handle non-blocking playback
    if (currentSong != 0) {
      if (now >= nextNoteTime) {
        const Note* melody = nullptr;
        int melodyLen = 0;
        int tempo = 120;

        if (currentSong == 1) {
          melody = mario_melody;
          melodyLen = mario_length;
          tempo = mario_tempo;
        } else if (currentSong == 2) {
          melody = despacito_melody;
          melodyLen = despacito_length;
          tempo = despacito_tempo;
        } else if (currentSong == 3) {
          melody = jingle_bells_melody;
          melodyLen = jingle_bells_length;
          tempo = jingle_bells_tempo;
        }

        if (melody && currentNoteIndex < melodyLen) {
          if (!isSilentGap) {
            uint16_t pitch = melody[currentNoteIndex].pitch;
            unsigned long noteDuration = 0;
            if (currentSong == 2) {
              noteDuration = melody[currentNoteIndex].duration; // Direct ms from Despacito.ino
            } else {
              noteDuration = 240000UL / (tempo * melody[currentNoteIndex].duration);
            }

            if (pitch > 0) {
              playTone(pitch);
            } else {
              stopTone();
            }
            nextNoteTime = now + (unsigned long)(noteDuration * 0.9);
            isSilentGap = true;
          } else {
            stopTone();
            unsigned long noteDuration = 0;
            if (currentSong == 2) {
              noteDuration = melody[currentNoteIndex].duration; // Direct ms
            } else {
              noteDuration = 240000UL / (tempo * melody[currentNoteIndex].duration);
            }

            nextNoteTime = now + (unsigned long)(noteDuration * 0.1);
            isSilentGap = false;
            currentNoteIndex++;
          }
        } else {
          stopSong();
        }
      }
      return;
    }

    // Normal buzzer one-shot sequences if no song is playing
    if (buzzerSeqStep == 1) {
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        if (buzzerGap > 0) {
          buzzerSeqStep = 2;
        } else {
          buzzerSeqStep = 0;
        }
        buzzerSeqStart = now;
      }
    } else if (buzzerSeqStep == 2) {
      stopTone();
      if (now - buzzerSeqStart >= (unsigned long)buzzerGap) {
        buzzerSeqStep = 3;
        buzzerSeqStart = now;
      }
    } else if (buzzerSeqStep == 3) {
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStep = 0;
      }
    }
  }
};

#endif
