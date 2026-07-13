#ifndef EMOTE_H
#define EMOTE_H

#include <FluxGarage_RoboEyes.h>
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
  bool showParamScreen = false;

  // Pointer to sensors to fetch values for overlay drawing
  Sensors *sensors = nullptr;

public:
  Emote()
      : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), eyes(display) {
  }

  void setShowParamScreen(bool show) {
    showParamScreen = show;
  }

  bool getShowParamScreen() const {
    return showParamScreen;
  }

  void drawMetricsOverlay(Adafruit_SSD1306 *disp) {
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
    // Screen width for eyes is 128 (full screen, eyes centered)
    eyes.begin(128, 64, 30);

    // Set eye configuration
    eyes.setWidth(30, 30);
    eyes.setHeight(32, 32);
    eyes.setBorderradius(8, 8);
    eyes.setSpacebetween(12);
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

    case DANCE_MODE:
      eyes.open();
      eyes.setMood(HAPPY);
      eyes.setSweat(false);
      eyes.setHFlicker(false);
      eyes.setAutoblinker(false); // No autoblinker while dancing
      eyes.setIdleMode(false);
      eyes.setPosition(DEFAULT);
      break;
    }
  }

  // Winking state variables
  bool isWinking = false;
  unsigned long winkEndTime = 0;

  void triggerWink() {
    isWinking = true;
    winkEndTime = millis() + 1000; // Wink for 1 second
    eyes.setAutoblinker(false);
    eyes.close(true, false); // Close left eye, open right eye
  }

  void triggerLaugh() { eyes.anim_laugh(); }

  void triggerConfused() { eyes.anim_confused(); }

  void drawParamScreen() {
    display.clearDisplay();

    // 1. Draw header
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 2);
    display.print(F("ENV STATUS"));
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

    // Fetch sensor readings
    float temp = (sensors != nullptr) ? sensors->getTemperature() : 0.0;
    float humid = (sensors != nullptr) ? sensors->getHumidity() : 0.0;
    float light = (sensors != nullptr) ? sensors->getLightLux() : 0.0;

    // Split into 3 columns:
    // Column 1: Temp (x: 0 to 41)
    // Column 2: Humid (x: 43 to 84)
    // Column 3: Light (x: 86 to 127)
    display.drawFastVLine(42, 13, 51, SSD1306_WHITE);
    display.drawFastVLine(85, 13, 51, SSD1306_WHITE);

    // --- Column 1: Temp ---
    display.setCursor(9, 16);
    display.print(F("TEMP"));
    
    display.setCursor(4, 28);
    display.setTextSize(2);
    display.print((int)round(temp));
    display.setTextSize(1);
    display.print(F("C"));
    
    // Progress Bar Temp: Range 0-50 C
    int tempBarVal = map(constrain((int)temp, 0, 50), 0, 50, 0, 30);
    display.drawRect(5, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(6, 49, tempBarVal, 5, SSD1306_WHITE);

    // --- Column 2: Humid ---
    display.setTextSize(1);
    display.setCursor(52, 16);
    display.print(F("HUMI"));
    
    display.setCursor(47, 28);
    display.setTextSize(2);
    display.print((int)round(humid));
    display.setTextSize(1);
    display.print(F("%"));
    
    // Progress Bar Humid: Range 0-100 %
    int humidBarVal = map(constrain((int)humid, 0, 100), 0, 100, 0, 30);
    display.drawRect(48, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(49, 49, humidBarVal, 5, SSD1306_WHITE);

    // --- Column 3: Light ---
    display.setTextSize(1);
    display.setCursor(94, 16);
    display.print(F("LIGHT"));
    
    int lightVal = (int)round(light);
    display.setCursor(90, 28);
    display.setTextSize(2);
    if (lightVal >= 1000) {
      display.setTextSize(1);
      display.setCursor(90, 32);
    }
    display.print(lightVal);
    display.setTextSize(1);
    
    // Progress Bar Light: Range 0-1000 lx
    int lightBarVal = map(constrain(lightVal, 0, 1000), 0, 1000, 0, 30);
    display.drawRect(91, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(92, 49, lightBarVal, 5, SSD1306_WHITE);

    display.display();
  }

  void update() {
    unsigned long now = millis();

    // Reset winking state when time is up
    if (isWinking && now >= winkEndTime) {
      isWinking = false;
      setExpression(currentState); // Restore normal mood/autoblink
    }

    if (showParamScreen) {
      drawParamScreen();
      return;
    }

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
      if (currentState == DANCE_MODE && !isWinking) {
        // Roll eyes in a circle during Dance Mode
        int directionIndex = (now / 200) % 8;
        int directions[] = {N, NE, E, SE, S, SW, W, NW};
        eyes.setPosition(directions[directionIndex]);
      }
      // Update standard RoboEyes animations
      eyes.update();
    }
  }
};

#endif
