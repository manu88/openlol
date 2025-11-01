#include "game_render.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_ctx.h"
#include "game_rules.h"
#include "geometry.h"
#include "menu.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void GameCopyPage(GameContext *gameCtx, uint16_t srcX, uint16_t srcY,
                  uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                  uint16_t srcPage, uint16_t dstPage) {
  if (w == 320) {
    return;
  }
  assert(gameCtx->loadedbitMap.data);
  renderCPSAt(gameCtx->pixBuf, gameCtx->loadedbitMap.data,
              gameCtx->loadedbitMap.imageSize, gameCtx->loadedbitMap.palette,
              destX, destY, w, h, 320, 200);
}

void renderDialog(GameContext *gameCtx) {
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

static void renderMap(GameContext *gameCtx) {
  UISetDefaultStyle();
  renderCPS(gameCtx->pixBuf, gameCtx->mapBackground.data,
            gameCtx->mapBackground.imageSize, gameCtx->mapBackground.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  char c[20] = "";
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf,
               MAP_SCREEN_EXIT_BUTTON_X + 2, MAP_SCREEN_BUTTONS_Y + 4, 50, c);

  GameContextGetLevelName(gameCtx, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, MAP_SCREEN_NAME_X,
               MAP_SCREEN_NAME_Y, 320 - MAP_SCREEN_NAME_Y, c);
  printf("%i\n", gameCtx->levelId);
}

static void renderPlayField(GameContext *gameCtx) {
  renderCPS(gameCtx->pixBuf, gameCtx->playField.data,
            gameCtx->playField.imageSize, gameCtx->playField.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
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

static void renderInventorySlot(GameContext *gameCtx, uint8_t slot,
                                uint16_t frameId) {
  assert(slot <= 9);
  SHPFrame frame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &frame, frameId);
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame,
               UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * (1 + slot)) + 2,
               UI_INVENTORY_BUTTON_Y, gameCtx->defaultPalette);
  SHPFrameRelease(&frame);
}

static void renderCharInventory(GameContext *gameCtx) {
  const SAVCharacter *character = gameCtx->chars + gameCtx->selectedChar;
  renderCPSAt(gameCtx->pixBuf, gameCtx->inventoryBackground.data,
              gameCtx->inventoryBackground.imageSize,
              gameCtx->inventoryBackground.palette, INVENTORY_SCREEN_X,
              INVENTORY_SCREEN_Y, INVENTORY_SCREEN_W, INVENTORY_SCREEN_H, 320,
              200);

  UISetStyle(UIStyle_Inventory);

  // char name
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 250, 10, 50,
               character->name);

  char c[16] = "";
  // force
  int y = 24;
  GameContextGetString(gameCtx, 0X4014, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 98, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterMight(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // protection
  y = 36;
  GameContextGetString(gameCtx, 0X4015, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 98, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterProtection(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // Fighter stats
  y = 62;
  GameContextGetString(gameCtx, 0X4016, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillFight(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // Rogue stats
  y = 72;
  GameContextGetString(gameCtx, 0X4017, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillRogue(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // Mage stats
  y = 82;
  GameContextGetString(gameCtx, 0X4018, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillMage(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // exit button
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 277, 104, 50, c);
}

static void renderInventory(GameContext *gameCtx) {
  for (int i = 0; i < 9; i++) {
    uint16_t index = (gameCtx->inventoryIndex + i) % INVENTORY_SIZE;
    uint16_t gameObjIndex = gameCtx->inventory[index];
    if (gameObjIndex == 0) {
      continue;
    }
    const GameObject *obj = gameCtx->itemsInGame + gameObjIndex;
    renderInventorySlot(
        gameCtx, i,
        GameContextGetItemSHPFrameIndex(gameCtx, obj->itemPropertyIndex));
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

void GameRender(GameContext *gameCtx) {
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  // SDL_RenderClear(gameCtx->renderer);
  if (gameCtx->state == GameState_MainMenu) {
    renderMainMenu(gameCtx);
    return;
  }
  if (gameCtx->state == GameState_ShowMap) {
    renderMap(gameCtx);
    return;
  }
  renderPlayField(gameCtx);

  renderLeftUIPart(gameCtx);
  if (gameCtx->fadeOutFrames) {
    gameCtx->fadeOutFrames--;
    // clearMazeZone(gameCtx);
  } else if (gameCtx->state == GameState_ShowInventory) {
    renderCharInventory(gameCtx);
  } else {
    GameRenderMaze(gameCtx);
    if (gameCtx->state == GameState_TimAnimation) {
      if (GameTimInterpreterRender(&gameCtx->timInterpreter) == 0) {
        GameContextSetState(gameCtx, GameState_PlayGame);
      }
    }
  }
  renderInventory(gameCtx);
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
