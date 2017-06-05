/*
 * mechanics.c
 *
 * Contains the logic behind updating the state of the game.
 */

#include <stdlib.h>

#include "mechanics.h"
#include "constants.h"

int *levelBuffer;

//Global game state holder
StateStruct gameState;

int generateNewFloorHeight() {
  int lastFloorHeight = levelBuffer[GAME_BUFFER_LENGTH - 1];
  // we must make sure our contiguous floor length is >= GAME_MIN_FLOOR_LENGTH
  if (levelBuffer[GAME_BUFFER_LENGTH - GAME_MIN_FLOOR_LENGTH - 1] != lastFloorHeight) {
    return lastFloorHeight;
  }

  // artificially adjust the frequency of new floor heights
  if (lastFloorHeight >= 0 && (random() % 10) < 8) {
    return lastFloorHeight;
  }

  // generate a new floor height
  int maxFloorHeight = GAME_HEIGHT - 3;
  if ((maxFloorHeight - lastFloorHeight) > GAME_MAX_HEIGHT_DIFF) {
    maxFloorHeight = lastFloorHeight + 2;
  }

  int newFloorHeight = (random() % (maxFloorHeight + 1) - 1);

  if (newFloorHeight < 0) {
    int i = GAME_BUFFER_LENGTH - GAME_MAX_GAP_LENGTH;
    for (;i < GAME_BUFFER_LENGTH; ++i) {
      if (levelBuffer[i] >= 0) {
        i = 0;
        break;
      }
    }
    // if we got to the end then we need to generate an actual tile
    if (i) {
      while(!newFloorHeight) {
        newFloorHeight = (random() % (maxFloorHeight + 1) - 1);
      }
    }
  }
  return newFloorHeight;
}

void scrollLevel() {
  if(--gameState.ticksLeftToScroll){
    return;
  }

  int i;
  for (i = 0; i < GAME_BUFFER_LENGTH - 1; ++i) {
    levelBuffer[i] = levelBuffer[i + 1];
  }
  levelBuffer[GAME_BUFFER_LENGTH - 1] = generateNewFloorHeight();
  gameState.ticksLeftToScroll = gameState.scrollSpeed * GAME_FRAMES_PER_SECOND;
}

void initializeGame() {
  levelBuffer = calloc(sizeof(int), GAME_BUFFER_LENGTH);
  // Start the procedural generation quicker (from index 99 on the buffer)
  int i = GAME_BUFFER_LENGTH - 100;
  while (--i) {
    levelBuffer[GAME_BUFFER_LENGTH - 1] = generateNewFloorHeight();
    int j;
    for (j = 0; j < GAME_BUFFER_LENGTH - 1; ++j) {
      levelBuffer[j] = levelBuffer[j + 1];
    }
  }
  gameState.scrollSpeed = 1.0 / 8;
  gameState.ticksLeftToScroll = gameState.scrollSpeed * GAME_FRAMES_PER_SECOND;
}
