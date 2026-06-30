#include "controller.h"

Controller robotController;

// Blynk callback to play/stop songs via Virtual Pin V5
BLYNK_WRITE(V5) {
  int songId = param.asInt();
  robotController.playSongBlynk(songId);
}

void setup() {
  robotController.begin();
}

void loop() {
  robotController.update();
}
