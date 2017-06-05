/*
 * constants.c
 *
 * Defines constants that control the universe! (of the game)
 */

#include "constants.h"

const int GAME_HEIGHT = 10;
const int GAME_BUFFER_LENGTH = 1000;
const int GAME_MIN_FLOOR_LENGTH = 6;
const int GAME_MAX_GAP_LENGTH = 6;
const int GAME_MAX_HEIGHT_DIFF = 2;

// Object char representations
const char GAME_PLAYER = '@';
const char GAME_FLOOR = '=';
const char GAME_AIR = ' ';

// Rendering constants
const int GAME_FRAMES_PER_SECOND = 60;
const int GAME_CHAR_COLUMN = 10;
