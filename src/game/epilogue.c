#include "epilogue.h"
#include "formats/format_lang.h"
#include "formats/format_tim.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "pak_file.h"
#include "tim.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
  PAKFile finale2;

  TIMContext timCtx;
  LangHandle lang; // LOLFINAL.DIP
} Epilogue;

// LOREFINL

static void EpilogueInit(GameContext *gameCtx, Epilogue *epilogue) {
  printf("EpilogueInit\n");
  TIMInit(&epilogue->timCtx, gameCtx, TIMInterpreterMode_Outro);
  PAKFileInit(&epilogue->finale2);
  assert(GameEnvironmentLoadLocalizedPak(&epilogue->finale2, "FINALE2.PAK"));

  {
    int index = PakFileGetEntryIndex(&epilogue->finale2, "LOLFINAL.TIM");
    uint8_t *data = PakFileGetEntryData(&epilogue->finale2, index);
    size_t dataSize = PakFileGetEntrySize(&epilogue->finale2, index);
    assert(TIMHandleFromBuffer(&epilogue->timCtx.scripts[0], data, dataSize));
  }
  {
    int index = PakFileGetEntryIndex(&epilogue->finale2, "LOLFINAL.DIP");
    uint8_t *data = PakFileGetEntryData(&epilogue->finale2, index);
    size_t dataSize = PakFileGetEntrySize(&epilogue->finale2, index);
    assert(LangHandleFromBuffer(&epilogue->lang, data, dataSize));
  }
}

static void EpilogueRelease(GameContext *gameCtx, Epilogue *epilogue) {
  printf("EpilogueRelease\n");
  PAKFileRelease(&epilogue->finale2);
}

static void EpilogueMainLoop(GameContext *gameCtx, Epilogue *epilogue) {
  printf("Start EpilogueMainLoop\n");

  TIMRun(&epilogue->timCtx, 0, 0);
  while (gameCtx->_shouldRun) {
    SDL_Event e = {0};
    int r =
        DisplayWaitMouseEvent(gameCtx->display, &e, gameCtx->conf.tickLength);
    if (r == 0) {
      gameCtx->_shouldRun = 0;
      break;
    }

    DisplayUpdate(gameCtx->display);
  }
}
int EpilogueShow(GameContext *gameCtx) {
  Epilogue epilogue = {0};
  EpilogueInit(gameCtx, &epilogue);
  EpilogueMainLoop(gameCtx, &epilogue);
  EpilogueRelease(gameCtx, &epilogue);
  return 0;
}
