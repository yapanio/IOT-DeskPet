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

  bool muted = false;

  int  currentSong       = 0;
  int  currentNoteIndex  = 0;
  unsigned long nextNoteTime = 0;
  bool isSilentGap       = false;

  unsigned long buzzerSeqStart = 0;
  int  buzzerSeqStep = 0;
  int  buzzerFreq    = 0;
  int  buzzerDur     = 0;
  int  buzzerGap     = 0;

  unsigned long lastAlarmToggle = 0;
  bool          alarmToggleState = false;

  const int BUZZER_VOLUME = 2;

  void playTone(int frequency) {
    if (frequency <= 0) { stopTone(); return; }
    ledcAttach(buzzerPin, frequency, 8);
    ledcWrite(buzzerPin, BUZZER_VOLUME);
  }
  void stopTone() {
    ledcWrite(buzzerPin, 0);
    ledcDetach(buzzerPin);
  }

 public:
  Actuators(int ledPin, int buzzerPin)
      : strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800), buzzerPin(buzzerPin) {}

  void begin() {
    strip.begin();
    strip.setBrightness(40);
    strip.show();

    pinMode(buzzerPin, OUTPUT);
    stopTone();
  }

  void setMuted(bool mute) {
    muted = mute;
    if (muted) {
      stopTone();
      stopSong();
    }
  }

  bool isMuted() const { return muted; }

  void setState(RobotState state) {
    if (currentState == state) return;

    currentState = state;

    stopTone();
    stopSong();
    buzzerSeqStep    = 0;
    lastAlarmToggle  = 0;
    alarmToggleState = false;
    muted            = false;
  }

  void playSong(int songId) {
    if (songId >= 1 && songId <= 3) {
      currentSong      = songId;
      currentNoteIndex = 0;
      nextNoteTime     = millis();
      isSilentGap      = false;
      stopTone();
    } else {
      stopSong();
    }
  }

  void stopSong() {
    currentSong = 0;
    stopTone();
  }

  bool isSongPlaying() const { return currentSong != 0; }

  void triggerSingleBeep(int frequency, int duration) {
    if (currentState == DANGER_FIRE || currentState == DANGER_HUMID) return;
    if (buzzerSeqStep != 0) return;

    buzzerFreq    = frequency;
    buzzerDur     = duration;
    buzzerGap     = 0;
    buzzerSeqStart = millis();
    buzzerSeqStep  = 1;
  }

  void triggerDoubleBeep(int frequency, int duration, int gap) {
    if (currentState == DANGER_FIRE || currentState == DANGER_HUMID) return;
    if (buzzerSeqStep != 0) return;

    buzzerFreq    = frequency;
    buzzerDur     = duration;
    buzzerGap     = gap;
    buzzerSeqStart = millis();
    buzzerSeqStep  = 1;
  }

  void update() {
    updateLEDs();
    updateBuzzer();
  }

 private:
  uint32_t rainbowColor(byte pos) {
    pos = 255 - pos;
    if (pos < 85)  return strip.Color(255 - pos * 3, 0, pos * 3);
    if (pos < 170) { pos -= 85; return strip.Color(0, pos * 3, 255 - pos * 3); }
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  }

  void updateLEDs() {
    unsigned long now = millis();
    if (now - lastLedUpdate < 30) return;
    lastLedUpdate = now;

    switch (currentState) {

      case DANGER_FIRE: {
        bool isOn = (now / 100) % 2;
        uint32_t color = isOn ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANGER_HUMID: {
        uint32_t color = strip.Color(255, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_HOT: {
        uint32_t color = strip.Color(178, 115, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_COLD: {
        uint32_t color = strip.Color(0, 178, 178);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_DARK: {
        bool isOn = (now / 500) % 2;
        uint32_t color = isOn ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case SLEEP_MODE: {
        uint32_t color = strip.Color(25, 0, 25);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case NORMAL_HAPPY: {
        float brightness = (sin(now * 2.0 * 3.14159265 / 3000.0) + 1.0) / 2.0;
        int greenValue = (int)(brightness * 255);
        uint32_t color = strip.Color(0, greenValue, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANCE_MODE: {
        int shift = (now / 5) % 256;
        for (int i = 0; i < LED_COUNT; i++) {
          byte colorPos = (i * 256 / LED_COUNT + shift) % 256;
          strip.setPixelColor(i, rainbowColor(colorPos));
        }
        strip.show();
        break;
      }
    }
  }

  void updateBuzzer() {
    unsigned long now = millis();

    if (currentState == DANGER_FIRE) {
      if (muted) { stopTone(); return; }
      if (isSongPlaying()) stopSong();

      if (now - lastAlarmToggle >= 120) {
        lastAlarmToggle  = now;
        alarmToggleState = !alarmToggleState;
        playTone(alarmToggleState ? 2500 : 1800);
      }
      return;
    }

    if (currentState == DANGER_HUMID) {
      if (muted) { stopTone(); return; }
      if (isSongPlaying()) stopSong();
      playTone(1000);
      return;
    }

    if (currentSong != 0) {
      if (muted) { stopSong(); return; }

      if (now >= nextNoteTime) {
        const Note* melody = nullptr;
        int melodyLen = 0;
        int tempo     = 120;

        if      (currentSong == 1) { melody = mario_melody;       melodyLen = mario_length;       tempo = mario_tempo; }
        else if (currentSong == 2) { melody = despacito_melody;   melodyLen = despacito_length;   tempo = despacito_tempo; }
        else if (currentSong == 3) { melody = jingle_bells_melody; melodyLen = jingle_bells_length; tempo = jingle_bells_tempo; }

        if (melody && currentNoteIndex < melodyLen) {
          unsigned long noteDuration = (currentSong == 2)
              ? melody[currentNoteIndex].duration
              : 240000UL / (tempo * melody[currentNoteIndex].duration);

          if (!isSilentGap) {
            uint16_t pitch = melody[currentNoteIndex].pitch;
            if (pitch > 0) playTone(pitch); else stopTone();
            
            nextNoteTime = now + (unsigned long)(noteDuration * 0.9);
            isSilentGap  = true;
          } else {
            stopTone();
            
            nextNoteTime = now + (unsigned long)(noteDuration * 0.1);
            isSilentGap  = false;
            currentNoteIndex++;
          }
        } else {
          stopSong();
        }
      }
      return;
    }

    if (buzzerSeqStep == 1) {
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStart = now;
        buzzerSeqStep  = (buzzerGap > 0) ? 2 : 0;
      }
    } else if (buzzerSeqStep == 2) {
      stopTone();
      if (now - buzzerSeqStart >= (unsigned long)buzzerGap) {
        buzzerSeqStep  = 3;
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
