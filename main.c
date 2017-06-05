/*
 * main.c
 *
 * File containing the logic for the main game loop yo.
 */

#include <time.h>
#include <signal.h>

#include "constants.h"
#include "rendering.h"
#include "mechanics.h"

volatile sig_atomic_t exiting = 0; // The flag to set on interrupt

void interruptHandler(int sig) {
  ++exiting;
}

unsigned int update() {
  // Timing start
  struct timespec startTime, endTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  unsigned int usecondsDiff;

  scrollLevel();

  // Timing finish
  clock_gettime(CLOCK_MONOTONIC, &endTime);
  usecondsDiff = (endTime.tv_sec - startTime.tv_sec) * 1e6;
  usecondsDiff += (endTime.tv_nsec - startTime.tv_nsec) / 1000;
  return usecondsDiff;
}

void loop() {
  unsigned int frameTime = (1.0 / GAME_FRAMES_PER_SECOND) * 1e6;
  while(!exiting) {
    unsigned int sleepTime = frameTime - update() - render();
    if (sleepTime <= 0) continue;
    struct timespec sleepNanoTime = {
      0,
      sleepTime * 1000
    };
    nanosleep(&sleepNanoTime, NULL);
  }
  endscreen();
}

int main() {
  signal(SIGINT, &interruptHandler);
  initGameScreen();
  initializeGame();
  loop();
  return 0;
}
