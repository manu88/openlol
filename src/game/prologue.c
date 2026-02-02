#include "prologue.h"
#include "display.h"
#include "formats/format_cps.h"
#include "formats/format_lang.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "pak_file.h"
#include "render.h"
#include "ui.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TXT_BUFFER_SIZE 128
static char txtBuffer[TXT_BUFFER_SIZE];

static const char *names[4] = {"Ak\'shel", "Michael", "Kieran", "Conrad"};
static const int attribs[4][3] = {
    {15, 8, 5},
    {6, 10, 15},
    {8, 6, 8},
    {10, 12, 10},
};

/*
Files:
INTRO9.PAK/CHARGEN.WSA : Richard talking
*/

typedef struct {
  CPSImage charBackground; // INTRO9.PAK/CHAR.CPS
  LangHandle lang;
  PAKFile intro09Pak;
  PAKFile startupPak;
  FNTHandle font9p;
} Prologue;

static void PrologueInit(GameContext *gameCtx, Prologue *prologue) {

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT9PN.FNT"));

    if (FNTHandleFromBuffer(&prologue->font9p, f.buffer, f.bufferSize) == 0) {
      printf("unable to get FONT9PN.FNT data\n");
    }
  }

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

  // left text
  LangHandleGetString(&prologue->lang, 57, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 38, 100,
               txtBuffer);

  LangHandleGetString(&prologue->lang, 58, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 48, 100,
               txtBuffer);
  LangHandleGetString(&prologue->lang, 59, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 58, 100,
               txtBuffer);
  LangHandleGetString(&prologue->lang, 60, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 68, 100,
               txtBuffer);

  LangHandleGetString(&prologue->lang, 61, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 78, 100,
               txtBuffer);

  // Names
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, 162,
                       names[0]);

  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, 162,
                       names[1]);

  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, 162,
                       names[2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, 162,
                       names[3]);

  int yOff = 172;
  // magic
  LangHandleGetString(&prologue->lang, 51, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[0][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[1][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[2][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[3][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       txtBuffer);

  yOff += 9;
  // protection
  LangHandleGetString(&prologue->lang, 53, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[0][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[1][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[2][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[3][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       txtBuffer);

  yOff += 9;
  // might
  LangHandleGetString(&prologue->lang, 55, txtBuffer, TXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[0][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[1][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[2][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       txtBuffer);

  snprintf(txtBuffer, TXT_BUFFER_SIZE, "%i", attribs[3][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       txtBuffer);
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
