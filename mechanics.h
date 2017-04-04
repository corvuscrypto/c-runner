/*
 * mechanics.h
 *
 * Header file for declaring typedefs and constants for use in mechanics.c
 */

#ifndef GAME_MECHANICS_H
#define GAME_MECHANICS_H

typedef struct {
  int score;
  double scrollSpeed;
} StateStruct;

extern StateStruct gameState;

#endif
