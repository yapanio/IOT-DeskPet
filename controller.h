#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "actuators.h"
#include "blynk_service.h"
#include "emote.h"
#include "robot_servo.h"
#include "robot_state.h"
#include "sensors.h"
#include <Arduino.h>

#define TOUCH_PIN  15
#define DHT_PIN    19
#define SERVO_PIN  14
#define LED_PIN     4
#define BUZZER_PIN 18

class Controller {
 private:
  Sensors      sensors;
  RobotServo   servo;
  Actuators    actuators;
  Emote        emote;
  BlynkService blynk;

  RobotState currentState = NORMAL_HAPPY;
  bool testModeActive = false;
  bool alarmMuted     = false;

  unsigned long lastTimeLightChecked  = 0;
  unsigned long lastTimeBuzzerSounded = 0;

  bool blynkTouchState = false;
  bool lastTouchState  = false;

  unsigned long touchStartTime  = 0;
  unsigned long lastTapTime     = 0;
  int           tapCount        = 0;
  bool          longPressDetected = false;

  bool       isDancing    = false;
  unsigned long danceEndTime = 0;
  RobotState preDanceState = NORMAL_HAPPY;

  bool showParamScreen = false;
  bool returnToParamScreenAfterDance = false;

 public:
  Controller()
      : sensors(DHT_PIN),
        servo(SERVO_PIN),
        actuators(LED_PIN, BUZZER_PIN) {}

  void begin() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    Serial.println(F("=========================================="));
    Serial.println(F("  Robot DeskPet bắt đầu thức dậy và khởi động..."));
    Serial.println(F("=========================================="));

    pinMode(TOUCH_PIN, INPUT);

    sensors.begin();
    servo.begin();
    actuators.begin();
    emote.begin(&sensors);
    blynk.begin();

    applyStateToSubsystems();

    lastTimeLightChecked  = millis();
    lastTimeBuzzerSounded = millis();

    Serial.println(F("Robot đã khởi động xong toàn bộ rồi!"));
  }

  void update() {
    sensors.update();

    if (isDancing) {
      bool songEnded      = !actuators.isSongPlaying();
      bool danceTimeIsUp  = (millis() >= danceEndTime);
      if (danceTimeIsUp || songEnded) {
        Serial.println(F("[Nhảy múa] Hết giờ rồi! Robot dừng nhảy để nghỉ ngơi."));
        stopDanceMode();
      }
    }

    if (!isDancing) {
      evaluateEnvironmentAndUpdateState();
    }

    updateTouchGesture();

    servo.update();
    actuators.update();
    emote.update();

    blynk.update(
      sensors.getTemperature(),
      sensors.getHumidity(),
      sensors.getHeatIndex(),
      sensors.getLightLux(),
      stateToString(currentState)
    );
  }

  void playSongBlynk(int songId) {
    if (songId >= 1 && songId <= 3) {
      triggerDanceMode(songId);
    } else {
      stopDanceMode();
    }
  }

  void setBlynkTouch(bool pressed) {
    blynkTouchState = pressed;
    Serial.print(F("[Blynk] Bé chạm ảo: "));
    Serial.println(blynkTouchState ? F("CHẠM VÀO") : F("THẢ TAY"));
  }

 private:
  void applyStateToSubsystems() {
    servo.setState(currentState);
    actuators.setState(currentState);
    emote.setExpression(currentState);
  }

  void changeState(RobotState newState, bool resetAlarm = true) {
    if (currentState == newState) return;

    Serial.print(F("[Cảm xúc đổi] Từ "));
    Serial.print(stateToString(currentState));
    Serial.print(F(" sang -> "));
    Serial.println(stateToString(newState));

    currentState = newState;
    if (resetAlarm) alarmMuted = false;

    applyStateToSubsystems();
  }

  void evaluateEnvironmentAndUpdateState() {
    float temp  = sensors.getTemperature();
    float humid = sensors.getHumidity();
    float feel  = sensors.getHeatIndex();
    float lux   = sensors.getLightLux();
    unsigned long now = millis();

    RobotState targetState = NORMAL_HAPPY;

    if (lux >= 150.0 || lux < 50.0) {
      lastTimeLightChecked = now;
    }

    if (temp > 42.0) {
      targetState = DANGER_FIRE;
    } else if (humid > 85.0) {
      targetState = DANGER_HUMID;
    } else if (temp > 35.0 || feel > 38.0) {
      targetState = WARNING_HOT;
    } else if (temp < 18.0 && humid < 36.0) {
      targetState = WARNING_COLD;
    } else if (lux < 50.0) {
      targetState = SLEEP_MODE;
    } else if (lux < 150.0) {
      if (now - lastTimeLightChecked > 600000) {
        targetState = WARNING_DARK;
      } else {
        targetState = (currentState == WARNING_DARK) ? WARNING_DARK : NORMAL_HAPPY;
      }
    } else {
      targetState = NORMAL_HAPPY;
    }

    if (targetState != currentState) {
      changeState(targetState, true);

      if (currentState == WARNING_HOT) {
        lastTimeBuzzerSounded = now - 300000;
        actuators.playSong(2);
      } else if (currentState == WARNING_COLD) {
        lastTimeBuzzerSounded = now;
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

  void triggerDanceMode(int songId = 1) {
    Serial.print(F("[Nhảy múa] Bắt đầu nhảy múa theo nhạc bài số: "));
    Serial.println(songId);

    isDancing     = true;
    danceEndTime  = millis() + 10000;
    preDanceState = currentState;

    changeState(DANCE_MODE, false);
    actuators.playSong(songId);
  }

  void stopDanceMode() {
    if (!isDancing) return;

    Serial.println(F("[Nhảy múa] Hết bài rồi, dừng nhảy múa thôi."));
    isDancing = false;
    actuators.stopSong();

    changeState(preDanceState, false);

    if (returnToParamScreenAfterDance) {
      showParamScreen = true;
      emote.setShowParamScreen(true);
      returnToParamScreenAfterDance = false;
    }
  }

  void handleSingleTap() {
    if (testModeActive) {
      Serial.println(F("[Test giả lập] Chạm 1 lần -> Đổi kịch bản giả lập thời tiết."));

      switch (currentState) {
        case NORMAL_HAPPY:
          sensors.setMock(true, 45.0, 50.0, 350.0);
          break;
        case DANGER_FIRE:
          sensors.setMock(true, 25.0, 90.0, 350.0);
          break;
        case DANGER_HUMID:
          sensors.setMock(true, 36.0, 50.0, 350.0);
          break;
        case WARNING_HOT:
          sensors.setMock(true, 15.0, 30.0, 350.0);
          break;
        case WARNING_COLD:
          sensors.setMock(true, 25.0, 55.0, 20.0);
          break;
        case SLEEP_MODE:
          sensors.setMock(true, 25.0, 55.0, 100.0);
          lastTimeLightChecked = millis() - 605000;
          break;
        case WARNING_DARK:
        default:
          sensors.setMock(false, 0, 0, 0);
          break;
      }

      alarmMuted = false;
      actuators.setMuted(false);
      return;
    }

    Serial.println(F("[Cảm ứng] Chạm 1 lần nhanh!"));

    bool alarmIsActive = !alarmMuted && (
        currentState == DANGER_FIRE  ||
        currentState == DANGER_HUMID ||
        actuators.isSongPlaying()
    );

    if (alarmIsActive) {
      Serial.println(F("[Cảm ứng] Im lặng: Tắt tiếng còi cảnh báo."));
      alarmMuted = true;
      actuators.setMuted(true);
    } else {
      showParamScreen = !showParamScreen;
      emote.setShowParamScreen(showParamScreen);
      Serial.print(F("[Cảm ứng] Xem màn hình thông số: "));
      Serial.println(showParamScreen ? F("BẬT") : F("TẮT"));

      actuators.triggerSingleBeep(1500, 100);
    }
  }

  void handleDoubleTap() {
    if (showParamScreen) {
      Serial.println(F("[Cảm ứng] Đang hiện màn hình thông số, bỏ qua chạm 2 lần."));
      return;
    }
    Serial.println(F("[Cảm ứng] Chạm 2 lần nhanh -> Nháy mắt + Lắc đầu chào bé!"));
    actuators.triggerDoubleBeep(2500, 60, 60);
    emote.triggerWink();
    servo.triggerGentleShake();
  }

  void handleTripleTap() {
    Serial.println(F("[Cảm ứng] Chạm 3 lần -> Bắt đầu nhảy múa nào!"));

    if (showParamScreen) {
      returnToParamScreenAfterDance = true;
      showParamScreen               = false;
      emote.setShowParamScreen(false);
    } else {
      returnToParamScreenAfterDance = false;
    }

    triggerDanceMode(1);
    danceEndTime = millis() + 5000;
  }

  void updateTouchGesture() {
    if (isDancing) return;

    unsigned long now = millis();

    bool isTouched = (digitalRead(TOUCH_PIN) == HIGH) || blynkTouchState;

    if (isTouched && !lastTouchState) {
      touchStartTime    = now;
      longPressDetected = false;
    }

    if (!isTouched && lastTouchState) {
      unsigned long pressDuration = now - touchStartTime;

      if (!longPressDetected && pressDuration > 50 && pressDuration < 600) {
        tapCount++;
        lastTapTime = now;
      }
    }

    if (isTouched && !longPressDetected) {
      unsigned long pressDuration = now - touchStartTime;
      if (pressDuration >= 2000) {
        longPressDetected = true;
        tapCount          = 0;

        testModeActive = !testModeActive;
        Serial.print(F("[Cảm ứng] Giữ lâu -> Chế độ Test: "));
        Serial.println(testModeActive ? F("MỞ") : F("TẮT"));

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

    if (tapCount > 0 && (now - lastTapTime > 400)) {
      if      (tapCount == 1) handleSingleTap();
      else if (tapCount == 2) handleDoubleTap();
      else if (tapCount >= 3) handleTripleTap();
      tapCount = 0;
    }

    lastTouchState = isTouched;
  }

  const char* stateToString(RobotState state) {
    switch (state) {
      case DANGER_FIRE:   return "DANGER_FIRE";
      case DANGER_HUMID:  return "DANGER_HUMID";
      case WARNING_HOT:   return "WARNING_HOT";
      case WARNING_COLD:  return "WARNING_COLD";
      case WARNING_DARK:  return "WARNING_DARK";
      case SLEEP_MODE:    return "SLEEP_MODE";
      case NORMAL_HAPPY:  return "NORMAL_HAPPY";
      case DANCE_MODE:    return "DANCE_MODE";
      default:            return "UNKNOWN";
    }
  }
};

#endif
