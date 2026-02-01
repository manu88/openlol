#include "prologue.h"
#include "display.h"
#include "game_ctx.h"
#include <stdio.h>

static void PrologueInit(GameContext *gameCtx) {}
static void PrologueRelease(GameContext *gameCtx) {}

static void PrologueMainLoop(GameContext *gameCtx);

void PrologueShow(GameContext *gameCtx) {
  PrologueInit(gameCtx);
  printf("start prologue Main loop\n");
  PrologueMainLoop(gameCtx);
  printf("done prologue Main loop\n");
  PrologueRelease(gameCtx);
}

static void PrologueMainLoop(GameContext *gameCtx) {
  while (1) {
    if (DisplayActiveDelay(gameCtx->display, gameCtx->conf.tickLength) == 0) {
      gameCtx->_shouldRun = 0;
      return;
    }
  }
}
