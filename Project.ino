#include "controller.h"

Controller robotController;

void setup() {
  robotController.begin();
}

void loop() {
  robotController.update();
}
