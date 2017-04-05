/*
 * rendering.c
 *
 * Contains code to render our game onto the terminal
 */

#include <curses.h>

void initGameScreen() {
  initscr();
  cbreak();
  curs_set(0);
  nodelay(stdscr, TRUE);
}

unsigned int render() {
  refresh();
  return 0;
}
