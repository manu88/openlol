#include "prologue.h"
#include "display.h"
#include "formats/format_cps.h"
#include "formats/format_lang.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "pak_file.h"
#include "render.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
Files:
INTRO9.PAK/CHARGEN.WSA : Richard talking
*/

typedef struct {
  CPSImage charBackground; // INTRO9.PAK/CHAR.CPS
  LangHandle lang;
  PAKFile intro09Pak;
  PAKFile startupPak;
  char txtBuffer[128];
} Prologue;

static void PrologueInit(GameContext *gameCtx, Prologue *prologue) {
  PAKFileInit(&prologue->intro09Pak);
  assert(GameEnvironmentLoadLocalizedPak(&prologue->intro09Pak, "INTRO9.PAK"));

  int index = PakFileGetEntryIndex(&prologue->intro09Pak, "CHAR.CPS");
  uint8_t *cpsData = PakFileGetEntryData(&prologue->intro09Pak, index);
  size_t cpsSize = PakFileGetEntrySize(&prologue->intro09Pak, index);
  CPSImageFromBuffer(&prologue->charBackground, cpsData, cpsSize);

  PAKFileInit(&prologue->startupPak);
  assert(GameEnvironmentLoadLocalizedPak(&prologue->startupPak, "STARTUP.PAK"));

  index = PakFileGetEntryIndex(&prologue->startupPak, "LOLINTRO.DIP");
  uint8_t *langData = PakFileGetEntryData(&prologue->startupPak, index);
  size_t langSize = PakFileGetEntrySize(&prologue->startupPak, index);
  LangHandleFromBuffer(&prologue->lang, langData, langSize);
}

static void PrologueRelease(GameContext *gameCtx, Prologue *prologue) {
  CPSImageRelease(&prologue->charBackground);

  PAKFileRelease(&prologue->intro09Pak);
  PAKFileRelease(&prologue->startupPak);
}

static void PrologueMainLoop(GameContext *gameCtx, Prologue *prologue);

void PrologueShow(GameContext *gameCtx) {
  Prologue prologue = {0};
  PrologueInit(gameCtx, &prologue);
  printf("start prologue Main loop\n");
  PrologueMainLoop(gameCtx, &prologue);
  printf("done prologue Main loop\n");
  PrologueRelease(gameCtx, &prologue);
}

static void RenderCharSelection(GameContext *gameCtx, Prologue *prologue) {
  renderCPS(gameCtx->display->pixBuf, prologue->charBackground.data,
            prologue->charBackground.imageSize,
            prologue->charBackground.palette, PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  // text on left side is at offset i=27
  LangHandleGetString(&prologue->lang, 27, prologue->txtBuffer, 128);
  printf("Text = '%s'\n", prologue->txtBuffer);
}

static void PrologueMainLoop(GameContext *gameCtx, Prologue *prologue) {

  RenderCharSelection(gameCtx, prologue);
  while (1) {
    if (DisplayActiveDelay(gameCtx->display, gameCtx->conf.tickLength) == 0) {
      gameCtx->_shouldRun = 0;
      return;
    }
    DisplayRender(gameCtx->display);
  }
}
