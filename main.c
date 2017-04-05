/*
 * main.c
 *
 * File containing the logic for the main game loop yo.
 */

#include <unistd.h>

#include "constants.h"
#include "rendering.h"

unsigned int update() {
  return 0;
}

void loop() {
  unsigned int sleepTime = (1 / GAME_FRAMES_PER_SECOND) * 1e6;
  while(1) {
    unsigned int updateTime = update();
    unsigned int renderTime = render();
    if (usleep(sleepTime - renderTime - updateTime)) return;
  }
}

int main() {
  initGameScreen();
  loop();
  return 0;
}
