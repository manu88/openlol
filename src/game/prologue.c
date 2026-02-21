#include "prologue.h"
#include "SDL_events.h"
#include "SDL_timer.h"
#include "audio.h"
#include "display.h"
#include "formats/format_cps.h"
#include "formats/format_lang.h"
#include "formats/format_shp.h"
#include "formats/format_wsa.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "geometry.h"
#include "pak_file.h"
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
INTRO9.PAK/FACE09.SHP ak'shel
INTRO9.PAK/FACE01.SHP mich'mich'
INTRO9.PAK/FACE08.SHP Miaou
INTRO9.PAK/FACE05.SHP Conrad

INTROVOC.PAK: voice files
INTROVOC.PAK/KING01.VOC : I have need of a champion...
INTROVOC.PAK/KING02.VOC : Well, have you decided
INTROVOC.PAK/KING03.VOC : Excellent, settle ...
*/

typedef enum {
  PrologueState_KingIntro = 0,
  PrologueState_CharSelection,
  PrologueState_CharTextBox,
  PrologueState_KingImpatient,
  PrologueState_KingOutro,
  PrologueState_Done,
} PrologueState;

typedef struct {
  PrologueState state;
  int selectedChar;
  CPSImage charBackground; // INTRO9.PAK/CHAR.CPS
  CPSImage details;        // INTRO9.PAK/BACKGRND.CPS
  LangHandle lang;         // LOLINTRO.DIP

  WSAHandle chargen;
  SHPHandle faces[4];
  int faceFrame[4];
  uint32_t nextFaceFrameTime[4];
  PAKFile voicePak;
  FNTHandle font9p;

  int isFirst;
  uint8_t *frameData;

  int gameEnvirScopeMark;
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
  prologue->gameEnvirScopeMark = GameEnvironmentAddScopeMark();
  assert(prologue->gameEnvirScopeMark);
  assert(GameEnvironmentPreloadLocalizedPak("INTRO9.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("STARTUP.PAK"));
  assert(GameEnvironmentPreloadLocalizedPak("INTROVOC.PAK"));

  PAKFileInit(&prologue->voicePak);
  assert(GameEnvironmentLoadLocalizedPak(&prologue->voicePak, "INTROVOC.PAK"));

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "CHAR.CPS"));
    assert(
        CPSImageFromBuffer(&prologue->charBackground, f.buffer, f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "BACKGRND.CPS"));
    assert(CPSImageFromBuffer(&prologue->details, f.buffer, f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "CHARGEN.WSA"));
    assert(WSAHandleFromBuffer(&prologue->chargen, f.buffer, f.bufferSize));
  }

  size_t frameDataSize =
      prologue->chargen.header.width * prologue->chargen.header.height;
  prologue->frameData = malloc(frameDataSize);

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FACE09.SHP"));
    assert(SHPHandleFromCompressedBuffer(&prologue->faces[0], f.buffer,
                                         f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FACE01.SHP"));
    assert(SHPHandleFromCompressedBuffer(&prologue->faces[1], f.buffer,
                                         f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FACE08.SHP"));
    assert(SHPHandleFromCompressedBuffer(&prologue->faces[2], f.buffer,
                                         f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FACE05.SHP"));
    assert(SHPHandleFromCompressedBuffer(&prologue->faces[3], f.buffer,
                                         f.bufferSize));
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "LOLINTRO.DIP"));
    assert(LangHandleFromBuffer(&prologue->lang, f.buffer, f.bufferSize));
  }
}

static void PrologueRelease(GameContext *gameCtx, Prologue *prologue) {
  for (int i = 0; i < 4; i++) {
    SHPHandleRelease(&prologue->faces[i]);
  }

  CPSImageRelease(&prologue->charBackground);
  CPSImageRelease(&prologue->details);
  PAKFileRelease(&prologue->voicePak);
  free(prologue->frameData);
  GameEnvironmentUnloadTopMark(prologue->gameEnvirScopeMark);
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

static void RenderCharSelection(GameContext *gameCtx, Prologue *prologue,
                                int showKingText) {

  DisplayRenderCPS(gameCtx->display, &prologue->charBackground, PIX_BUF_WIDTH,
                   PIX_BUF_HEIGHT);

  UISetStyle(UIStyle_Inventory);
  if (showKingText) {
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
static void KingIntroLoop(GameContext *gameCtx, Prologue *prologue);
static void KingOutroLoop(GameContext *gameCtx, Prologue *prologue);
static void kingIsImpatient(GameContext *gameCtx, Prologue *prologue);

static void PrologueMainLoop(GameContext *gameCtx, Prologue *prologue) {
  while (gameCtx->_shouldRun) {
    switch (prologue->state) {
    case PrologueState_KingIntro:
      KingIntroLoop(gameCtx, prologue);
      AudioSystemClearVoiceQueue(&gameCtx->audio);
      break;
    case PrologueState_CharSelection:
      CharSelectionLoop(gameCtx, prologue);
      AudioSystemClearVoiceQueue(&gameCtx->audio);
      break;
    case PrologueState_CharTextBox:
      CharTextBoxLoop(gameCtx, prologue);
      AudioSystemClearVoiceQueue(&gameCtx->audio);
      break;
    case PrologueState_KingOutro:
      KingOutroLoop(gameCtx, prologue);
      AudioSystemClearVoiceQueue(&gameCtx->audio);
      break;
    case PrologueState_KingImpatient:
      kingIsImpatient(gameCtx, prologue);
      AudioSystemClearVoiceQueue(&gameCtx->audio);
      break;
    case PrologueState_Done:
      return;
    default:
      assert(0);
    }
  }
}

static void selectedCharSpeech(GameContext *gameCtx, Prologue *prologue) {
  char file[10] = "";
  char c = 'A';
  int numFiles = 4;
  if (prologue->selectedChar == 1) {
    c = 'M';
    numFiles = 3;
  } else if (prologue->selectedChar == 2) {
    c = 'K';
    numFiles = 3;
  } else if (prologue->selectedChar == 3) {
    c = 'C';
    numFiles = 3;
  }
  int sequence[4] = {0};
  for (int i = 0; i < numFiles; i++) {
    snprintf(file, 10, "000%c%1i.VOC", c, i);
    int fileIndex = PakFileGetEntryIndex(&prologue->voicePak, file);
    sequence[i] = fileIndex;
  }
  AudioSystemPlayVoiceSequence(&gameCtx->audio, &prologue->voicePak, sequence,
                               numFiles);
}

static const uint8_t charInfoFrameTable[] = {
    0x0, 0x7, 0x8, 0x9, 0xA, 0xB, 0xA, 0x9, 0x8, 0x7, 0x0,
    0x0, 0x7, 0x8, 0x9, 0xA, 0xB, 0xA, 0x9, 0x8, 0x7, 0x0,
    0x0, 0x7, 0x8, 0x9, 0xA, 0xB, 0xA, 0x9, 0x8, 0x7};

static void CharTextBoxLoop(GameContext *gameCtx, Prologue *prologue) {

  selectedCharSpeech(gameCtx, prologue);

  DisplayRenderCPS(gameCtx->display, &prologue->charBackground, PIX_BUF_WIDTH,
                   PIX_BUF_HEIGHT);

  DisplayRenderCPSPart(gameCtx->display, &prologue->details, 0, 123, 0, 123,
                       PIX_BUF_WIDTH, 77, PIX_BUF_WIDTH);

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
  // copy the frame around face from background
  DisplayRenderCPSPart(gameCtx->display, &prologue->charBackground, 8, 127, 151,
                       124, 38, 38, 320);

  int frameIndex = 0;
  while (gameCtx->_shouldRun) {
    SDL_Event e = {0};
    int r =
        DisplayWaitMouseEvent(gameCtx->display, &e, gameCtx->conf.tickLength);
    if (r == 0) {
      gameCtx->_shouldRun = 0;
      break;
    } else if (r == 1) {
      Point pt = {e.button.x / SCREEN_FACTOR, e.button.y / SCREEN_FACTOR};

      if (zoneClicked(&pt, 87, 180, 40, 15)) {
        prologue->state = PrologueState_KingOutro;
        break;
      } else if (zoneClicked(&pt, 196, 180, 40, 15)) {
        prologue->selectedChar = -1;
        prologue->state = PrologueState_CharSelection;
        break;
      }
    }

    if (AudioSystemGetCurrentVoiceIndex(&gameCtx->audio) != -1) {
      SHPFrame charFrame = {0};
      int index = charInfoFrameTable[frameIndex];
      SHPHandleGetFrame(&prologue->faces[prologue->selectedChar], &charFrame,
                        index);
      assert(SHPFrameGetImageData(&charFrame));

      DisplayRenderSHP(gameCtx->display, &charFrame, 11, 130,
                       prologue->charBackground.palette);
      SHPFrameRelease(&charFrame);

      frameIndex++;
      if (frameIndex >= 32) {
        frameIndex = 0;
      }
    } else {
      SHPFrame charFrame = {0};
      SHPHandleGetFrame(&prologue->faces[prologue->selectedChar], &charFrame,
                        0);
      assert(SHPFrameGetImageData(&charFrame));

      DisplayRenderSHP(gameCtx->display, &charFrame, 11, 130,
                       prologue->charBackground.palette);
      SHPFrameRelease(&charFrame);
    }
    DisplayUpdate(gameCtx->display);
  }
}

static void updateCharacterBlinks(GameContext *gameCtx, Prologue *prologue) {
  uint32_t time = SDL_GetTicks();
  for (int i = 0; i < 4; i++) {
    if (time >= prologue->nextFaceFrameTime[i]) {
      SHPFrame charFrame = {0};
      SHPHandleGetFrame(&prologue->faces[i], &charFrame,
                        prologue->faceFrame[i]);
      assert(SHPFrameGetImageData(&charFrame));
      int x = 93;
      int y = 123;
      if (i == 1) {
        x = 151;
        y = 123;
      } else if (i == 2) {
        x = 209;
        y = 123;
      } else if (i == 3) {
        x = 268;
        y = 123;
      }
      x += 3;
      y += 4;
      DisplayRenderSHP(gameCtx->display, &charFrame, x, y,
                       prologue->charBackground.palette);
      SHPFrameRelease(&charFrame);

      prologue->faceFrame[i] = !prologue->faceFrame[i];
      if (prologue->faceFrame[i] == 0) {
        prologue->nextFaceFrameTime[i] = time + GetRandom(200, 300);
      } else {
        prologue->nextFaceFrameTime[i] = time + GetRandom(1000, 4000);
      }
    }
  }
}

static void KingOutroLoop(GameContext *gameCtx, Prologue *prologue) {
  // clear hero text and buttons, just keep the frame 0 face
  DisplayRenderCPSPart(gameCtx->display, &prologue->details, 0, 123, 0, 123,
                       PIX_BUF_WIDTH, 77, PIX_BUF_WIDTH);
  // hide buttons
  DisplayRenderCPSPart(gameCtx->display, &prologue->details, 87, 180, 87, 140,
                       150, 15, PIX_BUF_WIDTH);

  DisplayRenderCPSPart(gameCtx->display, &prologue->charBackground, 8, 127, 151,
                       124, 38, 38, 320);

  SHPFrame f = {0};
  SHPHandleGetFrame(&prologue->faces[prologue->selectedChar], &f, 0);
  assert(SHPFrameGetImageData(&f));

  DisplayRenderSHP(gameCtx->display, &f, 11, 130,
                   prologue->charBackground.palette);
  SHPFrameRelease(&f);
  int kingAudioSequenceId =
      PakFileGetEntryIndex(&prologue->voicePak, "KING03.VOC");
  assert(kingAudioSequenceId != -1);
  AudioSystemPlayVoiceSequence(&gameCtx->audio, &prologue->voicePak,
                               &kingAudioSequenceId, 1);

  LangHandleGetString(&prologue->lang, 64, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 38, 100,
               textBuffer);

  LangHandleGetString(&prologue->lang, 65, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 48, 100,
               textBuffer);
  LangHandleGetString(&prologue->lang, 66, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 58, 100,
               textBuffer);
  LangHandleGetString(&prologue->lang, 67, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 68, 100,
               textBuffer);

  LangHandleGetString(&prologue->lang, 68, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 78, 100,
               textBuffer);

  int animIndex = 0;

  while (gameCtx->_shouldRun && AudioSystemGetCurrentVoiceIndex(
                                    &gameCtx->audio) == kingAudioSequenceId) {
    size_t frameDataSize =
        prologue->chargen.header.width * prologue->chargen.header.height;
    if (animIndex == 0) {
      memset(prologue->frameData, 0, frameDataSize);
    }
    WSAHandleGetFrame(&prologue->chargen, animIndex, prologue->frameData, 1);
    DisplayRenderWSA(gameCtx->display, prologue->frameData, &prologue->chargen,
                     MAZE_COORDS_X, MAZE_COORDS_Y);

    animIndex += 1;
    if (animIndex > 4) {
      animIndex = 0;
    }

    DisplayUpdate(gameCtx->display);
    if (DisplayActiveDelay(gameCtx->display, gameCtx->conf.tickLength) == 0) {
      gameCtx->_shouldRun = 0;
      return;
    }
  }
  prologue->state = PrologueState_Done;
}

static void KingIntroLoop(GameContext *gameCtx, Prologue *prologue) {
  int kingAudioSequenceId =
      PakFileGetEntryIndex(&prologue->voicePak, "KING01.VOC");
  assert(kingAudioSequenceId != -1);
  AudioSystemPlayVoiceSequence(&gameCtx->audio, &prologue->voicePak,
                               &kingAudioSequenceId, 1);

  RenderCharSelection(gameCtx, prologue, 1);

  int animIndex = 0;

  while (gameCtx->_shouldRun && AudioSystemGetCurrentVoiceIndex(
                                    &gameCtx->audio) == kingAudioSequenceId) {

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

    size_t frameDataSize =
        prologue->chargen.header.width * prologue->chargen.header.height;
    if (animIndex == 0) {
      memset(prologue->frameData, 0, frameDataSize);
    }
    WSAHandleGetFrame(&prologue->chargen, animIndex, prologue->frameData, 1);
    DisplayRenderWSA(gameCtx->display, prologue->frameData, &prologue->chargen,
                     MAZE_COORDS_X, MAZE_COORDS_Y);

    animIndex += 1;
    if (animIndex > 4) {
      animIndex = 0;
    }

    updateCharacterBlinks(gameCtx, prologue);

    DisplayUpdate(gameCtx->display);
  }
  prologue->state = PrologueState_CharSelection;
}

static void kingIsImpatient(GameContext *gameCtx, Prologue *prologue) {
  int kingAudioSequenceId =
      PakFileGetEntryIndex(&prologue->voicePak, "KING02.VOC");
  assert(kingAudioSequenceId != -1);
  AudioSystemPlayVoiceSequence(&gameCtx->audio, &prologue->voicePak,
                               &kingAudioSequenceId, 1);

  DisplayRenderCPSPart(gameCtx->display, &prologue->charBackground, 0, 0, 0, 0,
                       110, 110, 320);
  LangHandleGetString(&prologue->lang, 62, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 38, 100,
               textBuffer);

  LangHandleGetString(&prologue->lang, 63, textBuffer, TEXT_BUFFER_SIZE);
  UIRenderText(&prologue->font9p, gameCtx->display->pixBuf, 8, 48, 100,
               textBuffer);

  int animIndex = 0;

  while (gameCtx->_shouldRun && AudioSystemGetCurrentVoiceIndex(
                                    &gameCtx->audio) == kingAudioSequenceId) {
    size_t frameDataSize =
        prologue->chargen.header.width * prologue->chargen.header.height;
    if (animIndex == 0) {
      memset(prologue->frameData, 0, frameDataSize);
    }
    WSAHandleGetFrame(&prologue->chargen, animIndex, prologue->frameData, 1);
    DisplayRenderWSA(gameCtx->display, prologue->frameData, &prologue->chargen,
                     MAZE_COORDS_X, MAZE_COORDS_Y);

    animIndex += 1;
    if (animIndex > 4) {
      animIndex = 0;
    }

    DisplayUpdate(gameCtx->display);
    if (DisplayActiveDelay(gameCtx->display, gameCtx->conf.tickLength) == 0) {
      gameCtx->_shouldRun = 0;
      return;
    }
  }
  prologue->state = PrologueState_CharSelection;
}

static void CharSelectionLoop(GameContext *gameCtx, Prologue *prologue) {
  RenderCharSelection(gameCtx, prologue, prologue->isFirst);
  prologue->isFirst = 0;

  uint32_t start = SDL_GetTicks();
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
    updateCharacterBlinks(gameCtx, prologue);
    DisplayUpdate(gameCtx->display);
    if (SDL_GetTicks() - start > 15000) {
      prologue->state = PrologueState_KingImpatient;
      return;
    }
  }
}
