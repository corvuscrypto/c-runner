/*
 * character.h
 *
 * Header file for the game character public members
 */

typedef struct {
  int jumpStart;
  int verticalDirection;
  int height;
  int ticksToNextHeight;
} Character;

extern Character gameChar;
