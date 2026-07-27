#include "controller.h"

Controller robotController;

BLYNK_WRITE(V5) {
  int songId = param.asInt();
  robotController.playSongBlynk(songId);
}

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
