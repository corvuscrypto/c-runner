/*
 * rendering.c
 *
 * Contains code to render our game onto the terminal
 */

#include <curses.h>
#include <signal.h>
#include <time.h>

#include "constants.h"
#include "mechanics.h"
#include "character.h"

volatile sig_atomic_t resized = 0;

void resizeHandler(int sig) {
  ++resized;
}

void endscreen() {
  endwin();
}

void printLevel() {
  //get screen width and height
  int width, height, row;
  getmaxyx(stdscr, height, width);
  move((height/2) - (GAME_HEIGHT/2), 0);
  for (row = GAME_HEIGHT; row >= 0; --row) {
    int col;
    for (col = 0; col < width; ++col) {
      char toAdd;
      if (row == ((gameChar.height) + 1) && col == GAME_CHAR_COLUMN) {
        toAdd = GAME_PLAYER;
      } else if (levelBuffer[col] == row) {
        toAdd = GAME_FLOOR;
      } else {
        toAdd = GAME_AIR;
      }
      addch(toAdd);
    }
  }
}

void initGameScreen() {
  initscr();
  cbreak();
  curs_set(0);
  nodelay(stdscr, TRUE);
  signal(SIGWINCH, &resizeHandler);
}

unsigned int render() {
  // Timing start
  struct timespec startTime, endTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  unsigned int usecondsDiff;

  if (resized) {
    erase();
    endwin();
    refresh();
    resized = 0;
  }
  printLevel();
  refresh();

  // Timing finish
  clock_gettime(CLOCK_MONOTONIC, &endTime);
  usecondsDiff = (endTime.tv_sec - startTime.tv_sec) * 1e6;
  usecondsDiff += (endTime.tv_nsec - startTime.tv_nsec) / 1000;
  return usecondsDiff;
}
