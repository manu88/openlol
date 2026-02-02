#include "prologue.h"
#include "SDL_events.h"
#include "display.h"
#include "formats/format_cps.h"
#include "formats/format_lang.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "geometry.h"
#include "pak_file.h"
#include "render.h"
#include "ui.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEXT_BUFFER_SIZE 128
static char textBuffer[TEXT_BUFFER_SIZE];

const char *charNames[4] = {"Ak\'shel", "Michael", "Kieran", "Conrad"};

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

typedef enum {
  PrologueState_CharSelection = 0,
  PrologueState_CharTextBox,
  PrologueState_Done,
} PrologueState;

typedef struct {
  PrologueState state;
  int selectedChar;
  CPSImage charBackground; // INTRO9.PAK/CHAR.CPS
  CPSImage details;        // INTRO9.PAK/BACKGRND.CPS
  LangHandle lang;
  PAKFile intro09Pak;
  PAKFile startupPak;
  FNTHandle font9p;

  int isFirst;
} Prologue;

static void PrologueInit(GameContext *gameCtx, Prologue *prologue) {
  memset(prologue, 0, sizeof(Prologue));
  prologue->isFirst = 1;
  prologue->selectedChar = -1;
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

  index = PakFileGetEntryIndex(&prologue->intro09Pak, "BACKGRND.CPS");
  cpsData = PakFileGetEntryData(&prologue->intro09Pak, index);
  cpsSize = PakFileGetEntrySize(&prologue->intro09Pak, index);
  assert(CPSImageFromBuffer(&prologue->details, cpsData, cpsSize));

  PAKFileInit(&prologue->startupPak);
  assert(GameEnvironmentLoadLocalizedPak(&prologue->startupPak, "STARTUP.PAK"));

  index = PakFileGetEntryIndex(&prologue->startupPak, "LOLINTRO.DIP");
  uint8_t *langData = PakFileGetEntryData(&prologue->startupPak, index);
  size_t langSize = PakFileGetEntrySize(&prologue->startupPak, index);
  LangHandleFromBuffer(&prologue->lang, langData, langSize);
}

static void PrologueRelease(GameContext *gameCtx, Prologue *prologue) {
  CPSImageRelease(&prologue->charBackground);
  CPSImageRelease(&prologue->details);

  PAKFileRelease(&prologue->intro09Pak);
  PAKFileRelease(&prologue->startupPak);
}

static void PrologueMainLoop(GameContext *gameCtx, Prologue *prologue);

int PrologueShow(GameContext *gameCtx) {
  Prologue prologue = {0};
  PrologueInit(gameCtx, &prologue);
  PrologueMainLoop(gameCtx, &prologue);
  int selected = prologue.selectedChar;
  PrologueRelease(gameCtx, &prologue);
  return selected;
}

static void RenderCharSelection(GameContext *gameCtx, Prologue *prologue) {
  printf("RenderCharSelection\n");
  renderCPS(gameCtx->display->pixBuf, prologue->charBackground.data,
            prologue->charBackground.imageSize,
            prologue->charBackground.palette, PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  UISetStyle(UIStyle_Inventory);

  if (prologue->isFirst) {
    // left text
    LangHandleGetString(&prologue->lang, 57, textBuffer, TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 38, 100,
                 textBuffer);

    LangHandleGetString(&prologue->lang, 58, textBuffer, TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 48, 100,
                 textBuffer);
    LangHandleGetString(&prologue->lang, 59, textBuffer, TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 58, 100,
                 textBuffer);
    LangHandleGetString(&prologue->lang, 60, textBuffer, TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 68, 100,
                 textBuffer);

    LangHandleGetString(&prologue->lang, 61, textBuffer, TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 78, 100,
                 textBuffer);
    prologue->isFirst = 0;
  }

  // Names
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, 162,
                       charNames[0]);

  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, 162,
                       charNames[1]);

  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, 162,
                       charNames[2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, 162,
                       charNames[3]);

  int yOff = 172;
  // magic
  LangHandleGetString(&prologue->lang, 51, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[0][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[1][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[2][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[3][0]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       textBuffer);

  yOff += 9;
  // protection
  LangHandleGetString(&prologue->lang, 53, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[0][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[1][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[2][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[3][1]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       textBuffer);

  yOff += 9;
  // might
  LangHandleGetString(&prologue->lang, 55, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 35, yOff, 50,
               textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[0][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 112, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[1][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 170, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[2][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 229, yOff,
                       textBuffer);

  snprintf(textBuffer, TEXT_BUFFER_SIZE, "%i", attribs[3][2]);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf, 287, yOff,
                       textBuffer);
}

static void CharSelectionLoop(GameContext *gameCtx, Prologue *prologue);
static void CharTextBoxLoop(GameContext *gameCtx, Prologue *prologue);

static void PrologueMainLoop(GameContext *gameCtx, Prologue *prologue) {
  while (gameCtx->_shouldRun) {
    switch (prologue->state) {
    case PrologueState_CharSelection:
      CharSelectionLoop(gameCtx, prologue);
      break;
    case PrologueState_CharTextBox:
      CharTextBoxLoop(gameCtx, prologue);
      break;
    case PrologueState_Done:
      printf("DONE \n");
      return;
    default:
      assert(0);
    }
  }
}

static void CharTextBoxLoop(GameContext *gameCtx, Prologue *prologue) {

  renderCPS(gameCtx->display->pixBuf, prologue->charBackground.data,
            prologue->charBackground.imageSize,
            prologue->charBackground.palette, PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  renderCPSPart(gameCtx->display->pixBuf, prologue->details.data,
                prologue->details.imageSize, prologue->details.palette, 0, 123,
                0, 123, PIX_BUF_WIDTH, 77, PIX_BUF_WIDTH);

  assert(prologue->selectedChar >= 0 && prologue->selectedChar < 4);
  int strIndex = 29 + (prologue->selectedChar * 5);

  for (int i = 0; i < 4; i++) {
    LangHandleGetString(&prologue->lang, strIndex + i, textBuffer,
                        TEXT_BUFFER_SIZE);
    UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 50, 127 + (9 * i),
                 PIX_BUF_WIDTH - 50, textBuffer);
  }

  // shall I serve you?
  LangHandleGetString(&prologue->lang, 69, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderTextCentered(&prologue->font9p, gameCtx->display->pixBuf,
                       PIX_BUF_WIDTH / 2, 168, textBuffer);
  DisplayRender(gameCtx->display);
  while (gameCtx->_shouldRun) {
    SDL_Event e = {0};
    int r =
        DisplayWaitMouseEvent(gameCtx->display, &e, gameCtx->conf.tickLength);
    if (r == 0) {
      gameCtx->_shouldRun = 0;
      return;
    } else if (r == 1) {
      Point pt = {e.button.x / SCREEN_FACTOR, e.button.y / SCREEN_FACTOR};

      if (zoneClicked(&pt, 87, 180, 40, 15)) {
        printf("Yes\n");
        prologue->state = PrologueState_Done;
        return;
      } else if (zoneClicked(&pt, 196, 180, 40, 15)) {
        printf("No\n");
        prologue->selectedChar = -1;
        prologue->state = PrologueState_CharSelection;
        return;
      }
    }
  }
}
static void CharSelectionLoop(GameContext *gameCtx, Prologue *prologue) {
  RenderCharSelection(gameCtx, prologue);
  DisplayRender(gameCtx->display);
  while (gameCtx->_shouldRun) {
    SDL_Event e = {0};
    int r =
        DisplayWaitMouseEvent(gameCtx->display, &e, gameCtx->conf.tickLength);
    if (r == 0) {
      gameCtx->_shouldRun = 0;
      return;
    } else if (r == 1) {
      Point pt = {e.button.x / SCREEN_FACTOR, e.button.y / SCREEN_FACTOR};

      if (zoneClicked(&pt, 93, 123, 37, 38)) {
        prologue->selectedChar = 0;
        prologue->state = PrologueState_CharTextBox;
        return;
      } else if (zoneClicked(&pt, 151, 123, 37, 38)) {
        prologue->selectedChar = 1;
        prologue->state = PrologueState_CharTextBox;
        return;
      } else if (zoneClicked(&pt, 210, 123, 37, 38)) {
        prologue->selectedChar = 2;
        prologue->state = PrologueState_CharTextBox;
        return;
      } else if (zoneClicked(&pt, 268, 123, 37, 38)) {
        prologue->selectedChar = 3;
        prologue->state = PrologueState_CharTextBox;
        return;
      }
    }
  }
}
