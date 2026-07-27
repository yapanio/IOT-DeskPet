#ifndef EMOTE_H
#define EMOTE_H

#include "robot_state.h"
#include "sensors.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

#define OLED_RESET -1

#define SCREEN_ADDRESS 0x3C

class Emote {
 private:
  Adafruit_SSD1306 display;
  RoboEyes<Adafruit_SSD1306> eyes;
  RobotState currentState = NORMAL_HAPPY;

  bool showParamScreen = false;

  bool isWinking = false;
  unsigned long winkEndTime = 0;

  Sensors* sensors = nullptr;

 public:
  Emote()
      : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
        eyes(display) {}

  void setShowParamScreen(bool show) { showParamScreen = show; }

  bool getShowParamScreen() const { return showParamScreen; }

  void begin(Sensors* sensorsPtr) {
    sensors = sensorsPtr;

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("[Màn hình] Lỗi: Khởi động màn hình OLED thất bại!"));
      return;
    }
    display.clearDisplay();
    display.display();

    eyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30);

    eyes.setWidth(30, 30);
    eyes.setHeight(32, 32);
    eyes.setBorderradius(8, 8);
    eyes.setSpacebetween(12);
    eyes.setAutoblinker(true, 3, 4);

    setExpression(NORMAL_HAPPY);
  }

  void setExpression(RobotState state) {
    currentState = state;

    switch (state) {

      case DANGER_FIRE:
        eyes.setAutoblinker(false);
        eyes.setIdleMode(false);
        break;

      case DANGER_HUMID:
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(true);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_HOT:
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_COLD:
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(true, 2);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_DARK:
        eyes.open();
        eyes.setMood(ANGRY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(N);
        break;

      case SLEEP_MODE:
        eyes.close();
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(false);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case NORMAL_HAPPY:
        eyes.open();
        eyes.setMood(HAPPY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(true, 3, 3);
        break;

      case DANCE_MODE:
        eyes.open();
        eyes.setMood(HAPPY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(false);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;
    }
  }

  void triggerWink() {
    isWinking    = true;
    winkEndTime  = millis() + 1000;
    eyes.setAutoblinker(false);
    eyes.close(true, false);
  }

  void triggerLaugh() { eyes.anim_laugh(); }

  void triggerConfused() { eyes.anim_confused(); }

  void drawParamScreen() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 2);
    display.print(F("ENV STATUS"));
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

    float temp  = (sensors != nullptr) ? sensors->getTemperature() : 0.0;
    float humid = (sensors != nullptr) ? sensors->getHumidity()    : 0.0;
    float light = (sensors != nullptr) ? sensors->getLightLux()    : 0.0;

    display.drawFastVLine(42, 13, 51, SSD1306_WHITE);
    display.drawFastVLine(85, 13, 51, SSD1306_WHITE);

    display.setCursor(9, 16);
    display.setTextSize(1);
    display.print(F("TEMP"));

    display.setCursor(4, 28);
    display.setTextSize(2);
    display.print((int)round(temp));
    display.setTextSize(1);
    display.print(F("C"));

    int tempBar = map(constrain((int)temp, 0, 50), 0, 50, 0, 30);
    display.drawRect(5, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(6, 49, tempBar, 5, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(52, 16);
    display.print(F("HUMI"));

    display.setCursor(47, 28);
    display.setTextSize(2);
    display.print((int)round(humid));
    display.setTextSize(1);
    display.print(F("%"));

    int humidBar = map(constrain((int)humid, 0, 100), 0, 100, 0, 30);
    display.drawRect(48, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(49, 49, humidBar, 5, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(94, 16);
    display.print(F("LIGHT"));

    int lightVal = (int)round(light);
    display.setCursor(90, 28);
    if (lightVal >= 1000) {
      display.setTextSize(1);
      display.setCursor(90, 32);
    } else {
      display.setTextSize(2);
    }
    display.print(lightVal);
    display.setTextSize(1);

    int lightBar = map(constrain(lightVal, 0, 1000), 0, 1000, 0, 30);
    display.drawRect(91, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(92, 49, lightBar, 5, SSD1306_WHITE);

    display.display();
  }

  void update() {
    unsigned long now = millis();

    if (isWinking && now >= winkEndTime) {
      isWinking = false;
      setExpression(currentState);
    }

    if (showParamScreen) {
      drawParamScreen();
      return;
    }

    if (currentState == DANGER_FIRE) {
      display.clearDisplay();

      display.drawLine(31, 20, 55, 44, SSD1306_WHITE);
      display.drawLine(55, 20, 31, 44, SSD1306_WHITE);

      display.drawLine(73, 20, 97, 44, SSD1306_WHITE);
      display.drawLine(97, 20, 73, 44, SSD1306_WHITE);

      display.drawLine(58, 48, 70, 48, SSD1306_WHITE);

      display.display();
      return;
    }

    if (currentState == DANCE_MODE && !isWinking) {
      int directionIndex = (now / 200) % 8;
      int directions[] = {N, NE, E, SE, S, SW, W, NW};
      eyes.setPosition(directions[directionIndex]);
    }

    eyes.update();
  }
};

#endif
