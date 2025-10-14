#include "game_render.h"
#include "render.h"
#include "renderer.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

void renderText(GameContext *gameCtx, int xOff, int yOff, int width,
                const char *text) {
  if (!text) {
    return;
  }
  int x = xOff;
  int y = yOff;
  for (int i = 0; i < strlen(text); i++) {
    drawChar2(gameCtx->pixBuf, &gameCtx->defaultFont, text[i], x, y);
    x += gameCtx->defaultFont.widthTable[(uint8_t)text[i]];
    if (x - xOff >= width) {
      x = xOff;
      y += gameCtx->defaultFont.maxHeight + 2;
    }
  }
}

void renderDialog(GameContext *gameCtx) {
  if (gameCtx->dialogText) {
    renderText(gameCtx, DIALOG_BOX_X + 5, DIALOG_BOX_Y + 2, DIALOG_BOX_W - 5,
               gameCtx->dialogText);
  }
}

static void drawDisabledOverlay(GameContext *gameCtx, int x, int y, int w,
                                int h) {
  void *data;
  int pitch;
  SDL_Rect r = {.x = x, .y = y, .w = w, .h = h};
  SDL_LockTexture(gameCtx->pixBuf, &r, &data, &pitch);
  for (int x = 0; x < r.w; x++) {
    for (int y = 0; y < r.h; y++) {
      if (x % 2 == 0 && y % 2 == 0) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      } else if (x % 2 == 1 && y % 2 == 1) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      }
    }
  }
  SDL_UnlockTexture(gameCtx->pixBuf);
}

void renderPlayField(GameContext *gameCtx) {
  renderCPS(gameCtx->pixBuf, gameCtx->playField.data,
            gameCtx->playField.imageSize, gameCtx->playField.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  clearMazeZone(gameCtx);

  if (gameCtx->controlDisabled) {
    drawDisabledOverlay(gameCtx, UI_TURN_LEFT_BUTTON_X, UI_TURN_LEFT_BUTTON_Y,
                        UI_DIR_BUTTON_W * 3, UI_DIR_BUTTON_H * 2);
    drawDisabledOverlay(gameCtx, UI_MENU_BUTTON_X, UI_MENU_BUTTON_Y,
                        UI_MENU_INV_BUTTON_W * 2, UI_MENU_INV_BUTTON_H);
  }
}

static void renderInventorySlot(GameContext *gameCtx, uint8_t slot,
                                uint16_t frameId) {
  assert(slot <= 9);
  SHPFrame frame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &frame, frameId);
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame,
               UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * (1 + slot)) + 2,
               UI_INVENTORY_BUTTON_Y, gameCtx->defaultPalette);
}

static void renderCharInventory(GameContext *gameCtx) {
  renderCPSAt(gameCtx->pixBuf, gameCtx->inventoryBackground.data,
              gameCtx->inventoryBackground.imageSize,
              gameCtx->inventoryBackground.palette, INVENTORY_SCREEN_X,
              INVENTORY_SCREEN_Y, INVENTORY_SCREEN_W, INVENTORY_SCREEN_H);
  char c[10] = "";
  LangHandleGetString(&gameCtx->lang, 51, c, sizeof(c));
  renderText(gameCtx, 277, 104, 50, c);

  renderText(gameCtx, 250, 10, 50, gameCtx->chars[gameCtx->selectedChar].name);
}

static void renderInventory(GameContext *gameCtx) {
  for (int i = 0; i < 9; i++) {
    uint16_t index = (gameCtx->inventoryIndex + i) % INVENTORY_SIZE;
    uint16_t itemId = gameCtx->inventory[index];
    if (itemId) {
      renderInventorySlot(gameCtx, i,
                          GameContextGetItemSHPFrameIndex(gameCtx, itemId));
    }
  }
}

static void renderCharFace(GameContext *gameCtx, uint8_t charId, int x) {
  SHPFrame frame = {0};
  assert(SHPHandleGetFrame(&gameCtx->charFaces[charId], &frame, 0));
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame, x, CHAR_FACE_Y,
               gameCtx->defaultPalette);
}

static void renderCharFaces(GameContext *gameCtx) {
  uint8_t numChars = GameContextGetNumChars(gameCtx);
  switch (numChars) {
  case 1:
    renderCharFace(gameCtx, 0, CHAR_FACE_0_1_X);
    break;
  case 2:
    renderCharFace(gameCtx, 0, CHAR_FACE_0_2_X);
    renderCharFace(gameCtx, 1, CHAR_FACE_1_2_X);
    break;
  case 3:
  default:
    assert(0);
  }
}

static void animateDialogZone(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(gameCtx->pixBuf, &rect, &data, &pitch);

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

  if (gameCtx->dialogBoxFrames <= (DIALOG_BOX_H2 - DIALOG_BOX_H)) {
    gameCtx->dialogBoxFrames++;
  }
  SDL_UnlockTexture(gameCtx->pixBuf);
}

void GameRender(GameContext *gameCtx) {
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gameCtx->renderer);
  renderPlayField(gameCtx);

  if (gameCtx->fadeOutFrames) {
    gameCtx->fadeOutFrames--;
    clearMazeZone(gameCtx);
  } else if (gameCtx->state == GameState_ShowInventory) {
    renderCharInventory(gameCtx);
  } else {
    if (gameCtx->imageTest.data) {
      renderCPS(gameCtx->pixBuf, gameCtx->imageTest.data,
                gameCtx->imageTest.imageSize, gameCtx->imageTest.palette,
                PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
    }
    GameRenderScene(gameCtx);
    if (gameCtx->state == GameState_TimAnimation) {
      if (GameTimAnimatorRender(&gameCtx->timAnimator) == 0) {
        GameContextSetState(gameCtx, GameState_PlayGame);
      }
    }
  }

  renderInventory(gameCtx);

  renderCharFaces(gameCtx);

  if (gameCtx->showBigDialog) {
    animateDialogZone(gameCtx);
  }

  renderDialog(gameCtx);
  // GameRenderMap(gameCtx, 640, 350);

  SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                   PIX_BUF_HEIGHT * SCREEN_FACTOR};
  assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->pixBuf, NULL, &dest) == 0);
  SDL_RenderPresent(gameCtx->renderer);
}
