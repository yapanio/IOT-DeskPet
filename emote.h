#ifndef EMOTE_H
#define EMOTE_H

#include "FluxGarage_RoboEyes.h"
#include "robot_state.h"
#include "sensors.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

class Emote {
private:
  Adafruit_SSD1306 display;
  RoboEyes<Adafruit_SSD1306> eyes;
  RobotState currentState = NORMAL_HAPPY;

  // Pointer to sensors to fetch values for overlay drawing
  static Sensors *sensors;

public:
  Emote()
      : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), eyes(display) {
  }

  static void drawMetricsOverlay(Adafruit_SSD1306 *disp) {
    if (sensors == nullptr)
      return;

    // Draw vertical separator line
    disp->drawFastVLine(97, 0, 64, SSD1306_WHITE);

    // Configure text rendering
    disp->setTextSize(1);
    disp->setTextColor(SSD1306_WHITE);

    // Row 1: Temperature (e.g., T / 25C)
    disp->setCursor(101, 4);
    disp->print(F("T"));
    disp->setCursor(101, 12);
    disp->print((int)round(sensors->getTemperature()));
    disp->print(F("C"));

    // Row 2: Humidity (e.g., H / 55%)
    disp->setCursor(101, 24);
    disp->print(F("H"));
    disp->setCursor(101, 32);
    disp->print((int)round(sensors->getHumidity()));
    disp->print(F("%"));

    // Row 3: Light (e.g., L / 150)
    disp->setCursor(101, 44);
    disp->print(F("L"));
    disp->setCursor(101, 52);
    disp->print((int)round(sensors->getLightLux()));
  }

  void begin(Sensors *sensorsPtr) {
    sensors = sensorsPtr;

    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      return;
    }

    display.clearDisplay();
    display.display();

    // Initialize RoboEyes
    // Screen width for eyes is 96 (leaving 32 pixels on the right for overlay)
    eyes.begin(96, 64, 30);
    eyes.onDrawOverlay = &drawMetricsOverlay;

    // Set eye configuration
    eyes.setWidth(26, 26); // Adjusted size to fit 96px width nicely
    eyes.setHeight(30, 30);
    eyes.setBorderradius(6, 6);
    eyes.setSpacebetween(10);
    eyes.setAutoblinker(true, 3, 4);

    setExpression(NORMAL_HAPPY);
  }

  void setExpression(RobotState state) {
    currentState = state;

    // Adjust RoboEyes parameters based on state
    switch (state) {
    case DANGER_FIRE:
      // Direct custom X_X drawing will be handled in update() instead of
      // RoboEyes
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
      eyes.setHFlicker(true, 2); // Shiver!
      eyes.setAutoblinker(true, 3, 4);
      eyes.setIdleMode(false);
      eyes.setPosition(DEFAULT);
      break;

    case WARNING_DARK:
      eyes.open();
      eyes.setMood(ANGRY); // Squinting
      eyes.setSweat(false);
      eyes.setHFlicker(false);
      eyes.setAutoblinker(true, 3, 4);
      eyes.setIdleMode(false);
      eyes.setPosition(N); // Look up
      break;

    case SLEEP_MODE:
      eyes.close(); // Closed eyes
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
      eyes.setIdleMode(true, 3, 3); // Look around randomly
      break;
    }
  }

  void triggerLaugh() { eyes.anim_laugh(); }

  void triggerConfused() { eyes.anim_confused(); }

  void update() {
    if (currentState == DANGER_FIRE) {
      // Direct drawing for X _ X face (bypassing RoboEyes)
      display.clearDisplay();

      // Draw left X (centered around x = 28, y = 32)
      display.drawLine(16, 20, 40, 44, SSD1306_WHITE);
      display.drawLine(40, 20, 16, 44, SSD1306_WHITE);

      // Draw right X (centered around x = 68, y = 32)
      display.drawLine(56, 20, 80, 44, SSD1306_WHITE);
      display.drawLine(80, 20, 56, 44, SSD1306_WHITE);

      // Draw mouth _ (centered around x = 48, y = 48)
      display.drawLine(43, 48, 53, 48, SSD1306_WHITE);

      // Draw metrics overlay on the right
      drawMetricsOverlay(&display);

      display.display();
    } else {
      // Update standard RoboEyes animations
      eyes.update();
    }
  }
};

// Define static member
Sensors *Emote::sensors = nullptr;

#endif
