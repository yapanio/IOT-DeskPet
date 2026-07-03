#include "controller.h"

Controller robotController;

// Blynk callback to play/stop songs via Virtual Pin V5
BLYNK_WRITE(V5) {
  int songId = param.asInt();
  robotController.playSongBlynk(songId);
}

// Blynk callback to simulate touch via Virtual Pin V6
BLYNK_WRITE(V6) {
  int pressed = param.asInt();
  robotController.setBlynkTouch(pressed == 1);
}

void setup() {
  robotController.begin();
}

void loop() {
  robotController.update();
}
