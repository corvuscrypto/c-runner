Welcome back to my idiocy-stricken, cringe-worthy foray into the world of game development! In this part of the series I finally find time to come back to my little runner game idea and get some rendering done as well as introduce you to my character!

## Timings and the loop revisited

First thing is first. I actually ended up not being okay with using `usleep`. The `nanosleep` really is much
better. I decided to keep the reported timings of update and render in microseconds since this resolution is fine and
will likely not cause integer overflow issues. Because this change and some others I made are pretty trivial and require
little understanding here is the updated `loop` function in its entirety so you can copy and paste:

```C
void loop() {
  unsigned int frameTime = (1.0 / GAME_FRAMES_PER_SECOND) * 1e6;
  while(1) {
    unsigned int sleepTime = frameTime - update() - render();
    if (sleepTime <= 0) continue;
    struct timespec sleepNanoTime = {
      0,
      sleepTime * 1000
    };
    nanosleep(&sleepNanoTime, NULL);
  }
}
```

Instead of importing `unistd.h` we now include `time.h`. Since I haven't actually implemented the timing yet, let's do that now.
Basically for both the `render()` and `update()` function we mark the current time using the system's monotonic clock at the beginning. Then we mark the time the same way at the end of the function. We return the time that has passed, in microseconds. For `update()` this looks like:

```C
unsigned int update() {
  // Timing start
  struct timespec startTime, endTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  unsigned int usecondsDiff;



  // Timing finish
  clock_gettime(CLOCK_MONOTONIC, &endTime);
  usecondsDiff = (endTime.tv_sec - startTime.tv_sec) * 1e6;
  usecondsDiff += (endTime.tv_nsec - startTime.tv_nsec) / 1000;
  return usecondsDiff;
}
```

You can just copy this over to `render()` (don't forget to `#include <time.h>` in `render.c` as well). Be sure that any actual function code goes between those and you are golden. That's pretty much it.

## A word about signals

Signals are great, they allow both the user and the kernel to interact with our program. If we are listening for them
that is. Because I expected a user to use the interrupt signal to stop this game, I wanted to implement a handler
for this to allow myself to perform cleanup (reset the terminal mode, free any dynamically allocated memory, etc.).
The typical signal handler in C looks like `void signalHandler(int)`. However, I don't want the handler to do
everything. That's usually a slippery slope into some pretty messy code. Instead I will use the handler to set an
atomic flag that my main update routine will check for.

```C
volatile sig_atomic_t exiting = 0; // The flag to set on interrupt

void interruptHandler(int sig) {
  ++exiting;
}
```

Now that's strange... why do the increment operation and not just set it to 1? Why not indeed. Just preference I guess.
Whatever you do in a signal handler, try to make sure you do things atomically and always be on the lookout for races in
code that relies on signals. The reason I added the `volatile` keyword is because otherwise there is a chance that the compiler will optimize away operations involving the flag if you enable optimizations (this totally happens and it can be infuriating to debug). In order to utilize this flag, I'm gonna change the `while(1)` loop condition to
`while(!exiting)` in my loop function. Then after the loop I will add the call to the cleanup function and then let the
program exit after. I haven't made the cleanup code yet, but we can wait on this for now. To set the signal we need
to import the signal library and call the function `signal(int, void (*)(int))` from our main routine. Now my main
looks like:

```C
int main(){  
  signal(SIGINT, &interruptHandler);
  initGameScreen();
  loop();
  return 0;
}
```

Like I said before, I also want to ensure that on program exit
the user has their terminal settings returned to normal. Therefore in my rendering file I want to create a function
called `endscreen`. In it I simply call the ncurses function `endwin()`
which resets the screen. The reason I put this into its own function is just to keep things organized in case I want to extend that functionality later on. I placed the call to endscreen right after the while loop in my loop function. However it could also be placed after the call to loop in the main function.

## Rendering levels

Ok so now that I have a way to exit gracefully, I still need to address the big invisible elephant on the screen. There's nothing there and nothing can be quite boring to look at. Since this game is a runner style of game I have a level that is made up of individual tiles that come one after the other. This to me seems like it can be easily modeled by a simple array. I can also represent gaps and the heights of the tiles in the game by just using integers. If I set gaps to be -1 and any other height to be its integer value this seems like it would work quite well. Remembering to properly declare a global variable in the header, I declared my array `int *levelBuffer` in mechanics.c.

This is a dynamic array which means I'll also need to initialize it at the start of the game. No problem. Because I'll need to initialize other things later, I created a function in mechanics.c to handle this called `initializeGame`. The code for now is very simple:

```C
void initializeGame() {
  levelBuffer = calloc(sizeof(int), GAME_BUFFER_LENGTH);
}
```

Don't forget to `#import <stdlib.h>` and `#import "constants.h"`. It's important to note that this call to calloc can fail and return NULL as a value. However I'm okay with this for now. The bonus of using calloc is that it initializes the array to 0 in all positions and that corresponds to the floor level of height 0. Convenient! I then place the call to this in my `main()` function. I do this right after the call to `initGameScreen()`. Don't forget to include `mechanics.h` in the `main.c` file.

Now for the really fun part; rendering this level of ours. My start on this begins in the `rendering.c` file with a function declaration of `void printLevel()`. I then placed a call to this before the `refresh()`; call in the `render()` function. Inside this new function I simply add characters to the ncurses buffer assuming I have a 2D-array with the number of rows equal to the `GAME_HEIGHT` and the number of columns equal to the screen width. The code I think will explain itself better:

```C
void printLevel(){
  //get screen width and height
  int width, height, row;
  getmaxyx(stdscr, height, width);
  move((height/2) - (GAME_HEIGHT/2), 0);
  for (row = GAME_HEIGHT; row >= 0; --row) {
    int col;
    for (col = 0; col < width; ++col) {
      char toAdd;
      if (levelBuffer[col] == row) {
        toAdd = GAME_FLOOR;
      } else {
        toAdd = GAME_AIR;
      }
      addch(toAdd);
    }
  }
}
```

To get the constants, remember to include `constants.h` and for the level buffer `mechanics.h`. The three unfamiliar function calls here, `getmaxyx(...)`, `move(...)`, and `addch(...)` come from the ncurses library. `getmaxyx` retrieves the stored terminal information about the width and height in terms of characters for a given screen (stdscr is the globally available default from ncurses). `move` moves the cursor to the specified location on the screen (in this case I move it to the top-left corner of the game). `addch` adds a character to the internal write buffer ncurses uses for the standard screen. This write buffer doesn't get output until a call to `refresh()` is performed (or some other functions ncurses offers that I won't get into). Since we put the call to this function right before the `refresh()` call in our render function, this should work. In case you missed it, I set the arguments in the `move` function call so that we have symmetrical vertical padding in the case that the terminal is taller than our 10-char height game.

COOL! Now we can compile our game and it should totally show the level rendered on the screen as a flat path!

![Awwww yiss](https://corvuscrypto.com/static/images/posts/5929f87ebb230d65f3e0b40e/part-2-1.png)

As someone who primarily develops backend reporting systems and tooling, I am not used to seeing results via a rendered graphic, so for me it is pretty damn cool to see steps of success in such a visible form!

## Handling window resizing

If you played around with the window size while rendering the scene just created, you will notice some straAaAaAaAaAange behavior. This is due to the screen redrawing without any kind of clearing on window resize. For a static screen size this is fine since when we redraw our scene it will always be completely overwritten. However, as you will notice this leaves ghosts on the screen when resizing to a smaller screen size. Now the easy way to handle this is to just put a call to the ncurses function `erase()` right before `printLevel`. This will solve the problem, but I would rather clear the whole terminal ONLY on resize. To do this we need a way to detect a resize event. Luckily most terminals emit a signal on window size changes. For Linux systems the signal is called `SIGWINCH`. We already know how to handle signals so we just need to implement a handler for this that sets a flag to check in our render function. The way I implemented is as such:

```C
volatile sig_atomic_t resized = 0;

void resizeHandler(int sig) {
  ++resized;
}
```

Then before my `printLevel` call I create an if statement that checks for this flag, then clears the screen if it is not 0 before resetting the flag to await the next resize event. It is not enough to just clear the screen, however. We have to also end the current screen and re-initialize it (this loads the new size information so that our `getmaxyx()` call will get the new, correct size). Since I am working with the default global `stdscr`, I can accomplish this with `endwin()` followed by `refresh()`, both placed after a call to `erase()`. To add the signal handler I put the `signal(SIGWINCH, &resizeHandler);` call first in my `initGameScreen()` function.

Now when the game compiles I can freely resize it and the window responds appropriately. Just to keep on the same page, here is the way my `render` function looks now:

```C
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
```

If you setup more than one window with ncurses, the handling of resizing must be handled in a way that sets the screens up appropriately for the new window sizes, but ultimately the basic setup for this looks very similar to what I've done here except that you probably wouldn't inline all the code in an `if` statement like I did.

## Making the game side-scroll

As is, this static display of `=` symbols is lame. I need to add some movement to spice things up! Turns out that with my way of modeling the level I can scroll by just shifting the array over one. This is easy enough by creating a function `void scrollLevel()` in `mechanics.c` with the following contents:

```C
void scrollLevel() {
  int i;
  for (i = 0; i < GAME_BUFFER_LENGTH - 1; ++i) {
    levelBuffer[i] = levelBuffer[i + 1];
  }
}
```

I think it's pretty clear what is happening here. We can just put a call to this function inside our `update()` function in `main.c`. Now the game can scroll, but unfortunately it will scroll so quickly the user will probably see a whole screen-width change in about a second, depending on the screen size. If you want to test this add `levelBuffer[GAME_BUFFER_LENGTH - 1] = 1;` at the end of the `scrollLevel()` function and behold your creation whizzing by at about `(GAME_BUFFER_LENGTH / GAME_FRAMES_PER_SECOND)` tiles per second. You monster.

We can slow this down by adding a ticker member in `StateStruct`. I decided to call this member `int ticksLeftToScroll`. At the very beginning of `scrollLevel` I  check the decrement of `gameState.ticksLeftToScroll`. If it is non-zero, I just return. Ezpz. Otherwise I continue to scroll the screen forward and then set the ticksLeftToScroll member to `(1.0 / gameState.scrollSpeed) * GAME_FRAMES_PER_SECOND` where `gameState.scrollSpeed` is the number of scrolls per second. I also set the struct member to this calculated value in `initializeGame`. For my starting scroll speed I picked 8 as my starting scroll speed (1/8th of a second before moving). By the way, the reason I didn't just make scrollSpeed represent the seconds before scrolling is because if I want to move the screen faster later, all I need to do is increment this value. If you want to see the scrolling, you can again add that line of code to set the last buffer value to 1, then you can also change the GAME_BUFFER_LENGTH to something shorter than the current value of 1000 to see the changes more quickly. The image below shows the result on my screen :D (the stuttering is just due to the terminal recording software).

![scroll harder plz](https://corvuscrypto.com/static/images/posts/5929f87ebb230d65f3e0b40e/scrolling.gif)

We are missing something however... What should *REALLY* go into the last index of the level buffer?

## Procedural generation of new level segments

Procedural generation is a phrase that makes things seems complex, but in my game's case, it's actually not. From the last post, here are the constraints for level generation again:

* At least 3 consecutive tiles on the same height.
* No tile can be higher than GAME_HEIGHT - 3
* No gaps larger than 6 consecutive tiles

And here's another constraint that I didn't mention last time but should have been:

* The new tile cannot be more than 2 rows higher than the previous tile.

For this last new constraint I created a new constant `const int GAME_MAX_HEIGHT_DIFF` and declared it to be 2. I can't set a constant for the second constraint because a `const` declaration that depends on another variable (even one that is constant) will elicit compiler complaints from gcc. Why not use defines? Because I hate them from past experiences debugging on embedded systems. Now that that's settled, the constraints I have are actually pretty easy to represent in code. I decided to make a new function in `mechanics.c` that returns the height of the new tile (or -1 if it is a gap) `int generateNewFloorHeight()`. First I grab the current last tile of the levelBuffer and store it locally. Then I check to see if we have enough tiles of the same height to satisfy our first condition. If this condition isn't met, then we return the current last floor height. I.e.

```C
int lastFloorHeight = levelBuffer[GAME_BUFFER_LENGTH - 1];
// we must make sure our contiguous floor length is >= GAME_MIN_FLOOR_LENGTH
if (levelBuffer[GAME_BUFFER_LENGTH - GAME_MIN_FLOOR_LENGTH] != lastFloorHeight) {
  return lastFloorHeight;
}
```

If we get past this part, then we get to generate a random number. To do this we need to utilize the `random()` function. We don't need to include anything since `random()` is part of `stdlib`. We want our random number to be in the interval [-1, GAME_MAX_FLOOR_HEIGHT]. The `random()` function returns a random `int` so to bring it to our range while maintaining its entropy, we use the equation `R* = (R % (highestValue - lowestValue) + lowestValue)` to generate the random number in our interval of [lowestValue, highestValue]. Normally you would want to seed the random function as well, but it defaults to a seed of 0 which for us is just fine. Since this equation doesn't satisfy our last constraint by itself, we normalize the upper bound if the difference from the last floor height is > 2. The code for this is as follows:

```C
// generate a new floor height
int maxFloorHeight = GAME_HEIGHT - 3;
if ((maxFloorHeight - lastFloorHeight) > GAME_MAX_HEIGHT_DIFF) {
  maxFloorHeight = lastFloorHeight + 2;
}

int newFloorHeight = (random() % (maxFloorHeight + 1) - 1);
```

After this, I check to see if the new floor is a gap (value of -1). This again is a simple if statement. If it is a gap then we need to see make sure we aren't gonna create a continuous gap longer than `GAME_MAX_GAP_LENGTH`. To do this we have to scan our array from index `GAME_BUFFER_LENGTH - GAME_MAX_GAP_LENGTH` to `GAME_BUFFER_LENGTH - 1`. I could implement heuristics like checking `lastFloorHeight` first before going to scan, but this scanning doesn't take much time and thus making such a heuristic is just overkill. K.I.S.S. applies (Google it if you haven't seen this acronym before and thank me). If, while scanning, we encounter a value that isn't a gap, then we are good to leave it as is. Otherwise we need to keep generating a new floor height until it is not a gap. The code for this is:

```C
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
    while(newFloorHeight < 0) {
      newFloorHeight = (random() % (maxFloorHeight + 1) - 1);
    }
  }
}
```

Then at the end we just return our `newFloorHeight`. In all, our `generateNewFloorHeight()` function is:

```C
int generateNewFloorHeight() {
  int lastFloorHeight = levelBuffer[GAME_BUFFER_LENGTH - 1];
  // we must make sure our contiguous floor length is >= GAME_MIN_FLOOR_LENGTH
  if (levelBuffer[GAME_BUFFER_LENGTH - GAME_MIN_FLOOR_LENGTH] != lastFloorHeight) {
    return lastFloorHeight;
  }

  // generate a new floor height
  int maxFloorHeight = GAME_MAX_FLOOR_HEIGHT;
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
      while(newFloorHeight < 0) {
        newFloorHeight = (random() % (maxFloorHeight + 1) - 1);
      }
    }
  }
  return newFloorHeight;
}
```

Now we can tie this into our `scrollLevel()` function by calling it toward the end of the function and setting its return value as the new last value in `levelBuffer`. Now we can compile and see our procedural generation in action! If you run this you might notice that most of our tiles seem to be the minimum length. This is totally not ideal. Let's say we want the frequency of actually changing the floor level to be roughly 30% of the time. We can then say that if we pass the first constraint check, we generate a random value in the interval of [0, 10]. If this random value is < 8 then we return the same floor level. To ensure we don't create gaps that are too long we can also ensure this only occurs if the `lastFloorHeight` is >= 0. We add the following just after the first if statement then in our `generateNewFloorHeight` function:

```C
// artificially adjust the frequency of new floor heights
if (lastFloorHeight >= 0 && (random() % 10) < 8) {
  return lastFloorHeight;
}
```

There! That looks muuuuuuch better. However, one thing I noticed is that a GAME_MIN_FLOOR_LENGTH of 6 is not enough for me. I changed this to 6 and enjoyed the results much more. Oh! and oooooone more thing. It would be boring for a player to wait for the first tile change to occur since my default `GAME_BUFFER_LENGTH` is 1000. There for after the `calloc` call in `initializeGame()` I set a new value from index 100 forward using `generateNewFloorHeight()`. I have to shift the array over for this to work each time because I'm too lazy to implement this part better. Behold the `initializeGame()` function after my hackish solution:

```C
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
```

Say what you want, but it works (please say it's terrible, because it is)!

## Behold the mighty Angelo!

With a crack of thunder sounded upon the release of his jump, my character can leap over the most difficult of obstacles. He is strong! He is resilient! He is... BRAZILIAN. (Hey Angelo :D I mentioned you in a blog post. #dealwithit). Also Angelo is an `@` symbol. In the final parts of this post lets draw him onto the screen. This is actually pretty simple. I basically want to keep the column that the character is rendered on constant. I therefore declare another constant `GAME_CHAR_COLUMN` and set it to 10. This is a pretty good position in my opinion. Since we are gonna add some logic to our character's actions later, we might as well set up a separate module to handle the representation of Angelo, `character.c` (and of course `character.h`). In the header I create a struct representation:

```C
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
```

Then I declare the global `gameChar` variable in the C file. And thus Angelo is born.

```C
/*
* character.c
*
* Main logic for controlling character state
*/

#include "character.h"

// global character for the game, Angelo.
Character gameChar;
```

For rendering him, the `height` member is what I will use to determine what height he is to be rendered upon. I also have a member called `ticksToNextHeight` which will be used when we implement jumping (coming soon?). We now change the code that determines which text character is to be added to the buffer in our `printLevel` function so that there is a condition where our player is what is added:

```C
if (row == ((gameChar.height) + 1) && col == GAME_CHAR_COLUMN) {
  toAdd = GAME_PLAYER;
} else if (levelBuffer[col] == row) {
  toAdd = GAME_FLOOR;
} else {
  toAdd = GAME_AIR;
}
```

Don't forget to include `character.h`! Now grab a beer amigo because we are done for now! When you compile and run the program you should now see Angelo whisking across the level, floating through the tiles like an ominous ghost. Great success!

![The mighty Angelo](https://corvuscrypto.com/static/images/posts/5929f87ebb230d65f3e0b40e/sofar.gif)
