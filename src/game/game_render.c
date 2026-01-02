#include "game_render.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "automap_render.h"
#include "formats/format_cps.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_char_inventory.h"
#include "game_ctx.h"
#include "geometry.h"
#include "menu.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void GameCopyPage(GameContext *gameCtx, uint16_t srcX, uint16_t srcY,
                  uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                  uint16_t srcPage, uint16_t dstPage) {
  if (w == 320) {
    return;
  }
  if (!gameCtx->loadedbitMap.data) {
    printf("[UNIMPLEMENTED] GameCopyPage: no loaded bitmap\n");
    return;
  }
  renderCPSAt(gameCtx->pixBuf, gameCtx->loadedbitMap.data,
              gameCtx->loadedbitMap.imageSize, gameCtx->loadedbitMap.palette,
              destX, destY, w, h, 320, 200);
}

static void renderDialog(GameContext *gameCtx) {
  UISetDefaultStyle();
  if (gameCtx->dialogText) {
    UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, DIALOG_BOX_X + 5,
                 DIALOG_BOX_Y + 2, DIALOG_BOX_W - 5, gameCtx->dialogText);
  }
}

static void drawDisabledOverlay(GameContext *gameCtx, SDL_Texture *texture,
                                int x, int y, int w, int h) {
  void *data;
  int pitch;
  SDL_Rect r = {.x = x, .y = y, .w = w, .h = h};
  SDL_LockTexture(texture, &r, &data, &pitch);
  for (int x = 0; x < r.w; x++) {
    for (int y = 0; y < r.h; y++) {
      if (x % 2 == 0 && y % 2 == 0) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      } else if (x % 2 == 1 && y % 2 == 1) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      }
    }
  }
  SDL_UnlockTexture(texture);
}

static void renderGameMenu(GameContext *gameCtx) {
  MenuRender(gameCtx->currentMenu, gameCtx, &gameCtx->defaultFont,
             gameCtx->pixBuf);
}

static void renderMainMenu(GameContext *gameCtx) {
  MenuRender(gameCtx->currentMenu, gameCtx, &gameCtx->defaultFont,
             gameCtx->pixBuf);
}

static void renderPlayField(GameContext *gameCtx) {
  renderCPS(gameCtx->pixBuf, gameCtx->playField.data,
            gameCtx->playField.imageSize, gameCtx->playField.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
#if 0            
  // left part
  renderCPSPart(gameCtx->pixBuf, gameCtx->playField.data,
                gameCtx->playField.imageSize, gameCtx->playField.palette, 0, 0,
                0, 0, MAZE_COORDS_X, PIX_BUF_HEIGHT, PIX_BUF_WIDTH);

  // bottom part
  renderCPSPart(gameCtx->pixBuf, gameCtx->playField.data,
                gameCtx->playField.imageSize, gameCtx->playField.palette,
                MAZE_COORDS_X, MAZE_COORDS_H, MAZE_COORDS_X, MAZE_COORDS_H,
                PIX_BUF_WIDTH - MAZE_COORDS_X, PIX_BUF_HEIGHT - MAZE_COORDS_H,
                PIX_BUF_WIDTH);

  // right part
  renderCPSPart(gameCtx->pixBuf, gameCtx->playField.data,
                gameCtx->playField.imageSize, gameCtx->playField.palette,
                MAZE_COORDS_X + MAZE_COORDS_W, 0, MAZE_COORDS_X + MAZE_COORDS_W,
                0, PIX_BUF_WIDTH - MAZE_COORDS_X + MAZE_COORDS_W,
                PIX_BUF_HEIGHT, PIX_BUF_WIDTH);
#endif
  renderCPSPart(gameCtx->pixBuf, gameCtx->playField.data,
                gameCtx->playField.imageSize, gameCtx->playField.palette,
                UI_MAP_BUTTON_X, UI_MAP_BUTTON_Y, 114, 65, UI_MAP_BUTTON_W,
                UI_MAP_BUTTON_H, 320);
  if (gameCtx->controlDisabled) {
    drawDisabledOverlay(gameCtx, gameCtx->pixBuf, UI_TURN_LEFT_BUTTON_X,
                        UI_TURN_LEFT_BUTTON_Y, UI_DIR_BUTTON_W * 3,
                        UI_DIR_BUTTON_H * 2);
    drawDisabledOverlay(gameCtx, gameCtx->pixBuf, UI_MENU_BUTTON_X,
                        UI_MENU_BUTTON_Y, UI_MENU_INV_BUTTON_W * 2,
                        UI_MENU_INV_BUTTON_H);
  }
}

static void drawLevelBar(GameContext *gameCtx, int startX, int startY,
                         SDL_Color col) {
  UIFillRect(gameCtx->pixBuf, startX, startY, 5, 32, col);
}

static void renderCharFace(GameContext *gameCtx, uint8_t charId, int x) {
  SHPFrame frame = {0};
  assert(SHPHandleGetFrame(&gameCtx->charFaces[charId], &frame, 0));
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame, x, CHAR_ZONE_Y + 1,
               gameCtx->defaultPalette);
  SHPFrameRelease(&frame);
}

static void renderCharZone(GameContext *gameCtx, uint8_t charId, int x) {
  UIFillRect(gameCtx->pixBuf, x, CHAR_ZONE_Y, CHAR_ZONE_W, CHAR_ZONE_H,
             (SDL_Color){0, 0, 0});
  renderCharFace(gameCtx, charId, x);
  const int xManaBar = 33;
  drawLevelBar(gameCtx, x + xManaBar, CHAR_ZONE_Y + 1,
               (SDL_Color){32, 40, 203});
  const int xLifeBar = 39;
  drawLevelBar(gameCtx, x + xLifeBar, CHAR_ZONE_Y + 1, (SDL_Color){4, 117, 24});

  UISetStyle(UIStyle_ManaLifeBars);
  char c[2] = "";
  GameContextGetString(gameCtx, 0X4253, c, 2);
  UIRenderText(&gameCtx->font6p, gameCtx->pixBuf, x + xManaBar, CHAR_ZONE_Y + 1,
               5, c);

  UISetTextStyle(UITextStyle_Highlighted);
  GameContextGetString(gameCtx, 0X4254, c, 2);
  UIRenderText(&gameCtx->font6p, gameCtx->pixBuf, x + xLifeBar, CHAR_ZONE_Y + 1,
               5, c);

  if (charId == gameCtx->selectedChar && gameCtx->selectedCharIsCastingSpell) {
    SHPFrame frame = {0};
    assert(SHPHandleGetFrame(&gameCtx->gameShapes, &frame, 73));
    SHPFrameGetImageData(&frame);
    drawSHPFrame(gameCtx->pixBuf, &frame, x + 44, CHAR_ZONE_Y,
                 gameCtx->defaultPalette);
    SHPFrameRelease(&frame);
  } else {

    {
      SHPFrame frame = {0};
      assert(SHPHandleGetFrame(&gameCtx->gameShapes, &frame, 54));
      SHPFrameGetImageData(&frame);
      drawSHPFrame(gameCtx->pixBuf, &frame, x + 44, CHAR_ZONE_Y,
                   gameCtx->defaultPalette);
      SHPFrameRelease(&frame);

      if (gameCtx->controlDisabled) {
        drawDisabledOverlay(gameCtx, gameCtx->pixBuf, x + 44, CHAR_ZONE_Y, 22,
                            18);
      }
    }
    {
      SHPFrame frame = {0};
      assert(SHPHandleGetFrame(&gameCtx->gameShapes, &frame, 72));
      SHPFrameGetImageData(&frame);
      drawSHPFrame(gameCtx->pixBuf, &frame, x + 44, CHAR_ZONE_Y + 16,
                   gameCtx->defaultPalette);
      SHPFrameRelease(&frame);
      if (gameCtx->controlDisabled) {
        drawDisabledOverlay(gameCtx, gameCtx->pixBuf, x + 44, CHAR_ZONE_Y + 16,
                            22, 18);
      }
    }
  }
  UIResetTextStyle();
  if (charId == gameCtx->selectedChar) {
    UIStrokeRect(gameCtx->pixBuf, x, CHAR_ZONE_Y, CHAR_ZONE_W, CHAR_ZONE_H,
                 (SDL_Color){117, 115, 145});
  }
}

static void renderLeftUIPart(GameContext *gameCtx) {
  char t[8] = "";
  UISetDefaultStyle();
  snprintf(t, 8, "%i", gameCtx->credits);
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, UI_CREDITS_X + 5,
               UI_CREDITS_Y + 2, 20, t);
}

static void renderCharFaces(GameContext *gameCtx) {
  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    renderCharZone(gameCtx, 0, CHAR_ZONE_0_1_X);
    break;
  case 2:
    renderCharZone(gameCtx, 0, CHAR_ZONE_0_2_X);
    renderCharZone(gameCtx, 1, CHAR_ZONE_1_2_X);
    break;
  case 3:
    renderCharZone(gameCtx, 0, CHAR_ZONE_0_3_X);
    renderCharZone(gameCtx, 1, CHAR_ZONE_1_3_X);
    renderCharZone(gameCtx, 2, CHAR_ZONE_2_3_X);
    break;
  default:
    assert(0);
  }
}

static void animateDialogZoneOnce(GameContext *gameCtx, void *data, int pitch) {
  // copy outline
  int offset = gameCtx->dialogBoxFrames;
  const int size = 20;
  for (int y = DIALOG_BOX_H - size; y < DIALOG_BOX_H; y++) {
    const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
    uint32_t *rowDest = (unsigned int *)((char *)data + pitch * (y + offset));
    for (int x = 0; x < DIALOG_BOX_W; x++) {
      rowDest[x] = rowSource[x];
    }
  }

  if (gameCtx->dialogBoxFrames >= size) {
    // need to copy the dialog background
    int size = DIALOG_BOX_H - 1;
    if (gameCtx->dialogBoxFrames == DIALOG_BOX_H2 - DIALOG_BOX_H) {
      size++;
    }
    for (int y = 1; y < size; y++) {
      const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
      uint32_t *rowDest =
          (unsigned int *)((char *)data + pitch * (y + (DIALOG_BOX_H - 2)));
      for (int x = 0; x < DIALOG_BOX_W; x++) {
        rowDest[x] = rowSource[x];
      }
    }
  }
}

static void showBigDialogZone(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(gameCtx->pixBuf, &rect, &data, &pitch);
  for (int i = 0; i < 1 + DIALOG_BOX_H2 - DIALOG_BOX_H; i++) {
    animateDialogZoneOnce(gameCtx, data, pitch);
  }
  SDL_UnlockTexture(gameCtx->pixBuf);
}

static void growDialogBox(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(gameCtx->pixBuf, &rect, &data, &pitch);
  animateDialogZoneOnce(gameCtx, data, pitch);
  SDL_UnlockTexture(gameCtx->pixBuf);

  if (gameCtx->dialogBoxFrames <= DIALOG_BOX_H2 - DIALOG_BOX_H) {
    gameCtx->dialogBoxFrames++;
  } else {
    // done
    assert(gameCtx->prevState != GameState_Invalid);
    gameCtx->state = gameCtx->prevState;
    gameCtx->showBigDialog = 1;
  }
}

static void shrinkDialogBox(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(gameCtx->pixBuf, &rect, &data, &pitch);
  for (int i = 0; i < 1 + DIALOG_BOX_H2 + gameCtx->dialogBoxFrames; i++) {
    animateDialogZoneOnce(gameCtx, data, pitch);
  }
  SDL_UnlockTexture(gameCtx->pixBuf);

  if (gameCtx->dialogBoxFrames > 0) {
    gameCtx->dialogBoxFrames--;
  } else {
    assert(gameCtx->prevState != GameState_Invalid);
    gameCtx->state = gameCtx->prevState;
    gameCtx->showBigDialog = 0;
  }
}

static void renderExitButton(GameContext *gameCtx) {
  UIDrawTextButton(&gameCtx->defaultFont, gameCtx->pixBuf,
                   UI_SCENE_EXIT_BUTTON_X, UI_SCENE_EXIT_BUTTON_Y,
                   UI_SCENE_EXIT_BUTTON_W, UI_SCENE_EXIT_BUTTON_H, "EXIT");

  if (gameCtx->exitSceneButtonDisabled) {
    drawDisabledOverlay(gameCtx, gameCtx->pixBuf, UI_SCENE_EXIT_BUTTON_X,
                        UI_SCENE_EXIT_BUTTON_Y, UI_SCENE_EXIT_BUTTON_W,
                        UI_SCENE_EXIT_BUTTON_H);
  }
}

void DimRect(SDL_Texture *texture, int startX, int startY, int w, int h) {
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      uint8_t r = ((row[x] & 0X00FF0000) >> 16) / 1.5;
      uint8_t g = ((row[x] & 0X0000FF00) >> 8) / 1.5;
      uint8_t b = ((row[x] & 0X000000FF)) / 1.5;
      row[x] = 0XFF000000 + (r << 0X10) + (g << 0X8) + b;
    }
  }
  SDL_UnlockTexture(texture);
}

void GameRenderRenderSceneFade(GameContext *gameCtx) {
  DimRect(gameCtx->pixBuf, MAZE_COORDS_X, MAZE_COORDS_Y, MAZE_COORDS_W,
          MAZE_COORDS_H);
}

void GameRender(GameContext *gameCtx) {
  // SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  //  SDL_RenderClear(gameCtx->renderer);
  if (gameCtx->state == GameState_MainMenu) {
    renderMainMenu(gameCtx);
    return;
  }
  if (gameCtx->state == GameState_ShowMap) {
    AutomapRender(gameCtx);
    return;
  }
  renderPlayField(gameCtx);

  renderLeftUIPart(gameCtx);
  if (gameCtx->state == GameState_ShowInventory) {
    renderCharInventory(gameCtx);
  } else {
    GameRenderMaze(gameCtx);
    if (gameCtx->state == GameState_TimAnimation) {
      if (GameTimInterpreterRender(&gameCtx->timInterpreter) == 0) {
        GameContextSetState(gameCtx, GameState_PlayGame);
      }
    }
  }
  renderInventoryStrip(gameCtx);
  renderCharFaces(gameCtx);

  if (gameCtx->state == GameState_GameMenu) {
    renderGameMenu(gameCtx);
    return;
  }

  if (gameCtx->state == GameState_GrowDialogBox) {
    growDialogBox(gameCtx);
  } else if (gameCtx->state == GameState_ShrinkDialogBox) {
    shrinkDialogBox(gameCtx);
  } else if (gameCtx->showBigDialog) {
    showBigDialogZone(gameCtx);
  }
  if (gameCtx->drawExitSceneButton) {
    renderExitButton(gameCtx);
  }

  if (gameCtx->state != GameState_GrowDialogBox &&
      gameCtx->state != GameState_ShrinkDialogBox) {
    if (gameCtx->buttonText[0]) {
      UIDrawTextButton(&gameCtx->defaultFont, gameCtx->pixBuf, DIALOG_BUTTON1_X,
                       DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W, DIALOG_BUTTON_H,
                       gameCtx->buttonText[0]);
    }

    if (gameCtx->buttonText[1]) {
      UIDrawTextButton(&gameCtx->defaultFont, gameCtx->pixBuf, DIALOG_BUTTON2_X,
                       DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W, DIALOG_BUTTON_H,
                       gameCtx->buttonText[1]);
    }
    if (gameCtx->buttonText[2]) {
      UIDrawTextButton(&gameCtx->defaultFont, gameCtx->pixBuf, DIALOG_BUTTON3_X,
                       DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W, DIALOG_BUTTON_H,
                       gameCtx->buttonText[2]);
    }
    renderDialog(gameCtx);
  }
}

void GameRenderResetDialog(GameContext *gameCtx) { gameCtx->dialogText = NULL; }

static inline int stringHasCharName(const char *s, int startIndex) {
  size_t strLen = strlen(s);
  int i = 0;
  if (startIndex) {
    i = startIndex + 1;
  }
  for (; i < strLen; i++) {
    if (s[i] == '%') {
      if (i + 1 >= strLen) {
        return -1;
      }
      char next = s[i + 1];
      if (next == 'n') {
        return i;
      } // XXX handle other special string aliasing types here if any
    }
  }
  return -1;
}

static inline char *stringReplaceHeroNameAt(GameContext *gameCtx, char *string,
                                            size_t bufferSize, int index) {
  const char *heroName = gameCtx->chars[0].name;
  size_t totalNewSize = strlen(string) - 2 + strlen(heroName) + 1;
  assert(totalNewSize < bufferSize);
  char *newStr = malloc(totalNewSize);
  memcpy(newStr, string, index);
  newStr[index] = 0;

  strcat(newStr, heroName);
  strcat(newStr, string + index + 2);
  return newStr;
}

void GameRenderSetDialogF(GameContext *gameCtx, int stringId, ...) {
  GameContextGetString(gameCtx, stringId, gameCtx->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  char *format = strdup(gameCtx->dialogTextBuffer);

  int placeholderIndex = 0;
  do {
    placeholderIndex = stringHasCharName(format, placeholderIndex);
    if (placeholderIndex != -1) {
      char *newFormat = stringReplaceHeroNameAt(
          gameCtx, format, DIALOG_BUFFER_SIZE, placeholderIndex);
      free(format);
      format = newFormat;
    }
  } while (placeholderIndex != -1);

  va_list args;
  va_start(args, stringId);
  vsnprintf(gameCtx->dialogTextBuffer, DIALOG_BUFFER_SIZE, format, args);
  va_end(args);
  free(format);
  gameCtx->dialogText = gameCtx->dialogTextBuffer;
}
