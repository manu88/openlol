#include "game_render.h"
#include "SDL_pixels.h"
#include "automap_render.h"
#include "display.h"
#include "formats/format_cps.h"
#include "formats/format_shp.h"
#include "game_char_inventory.h"
#include "game_ctx.h"
#include "game_render.h"
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
  if (!gameCtx->display->loadedbitMap.data) {
    printf("[UNIMPLEMENTED] GameCopyPage: no loaded bitmap\n");
    return;
  }
  DisplayRenderCPSAt(gameCtx->display, &gameCtx->display->loadedbitMap, destX,
                     destY, w, h, 320, 200);
  printf("render bitmap srcX=%i srcY=%i dstX=%i dstY=%i w=%i h=%i\n", srcX,
         srcY, destX, destY, w, h);
  gameCtx->display->showBitmap = 1;
}

static void renderDialog(GameContext *gameCtx) {
  UISetDefaultStyle();
  if (gameCtx->display->dialogText) {
    UIRenderText(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                 DIALOG_BOX_X + 5, DIALOG_BOX_Y + 2, DIALOG_BOX_W - 5,
                 gameCtx->display->dialogText);
  }
}

static void renderGameMenu(GameContext *gameCtx) {
  MenuRender(gameCtx->display->currentMenu, gameCtx,
             &gameCtx->display->defaultFont, gameCtx->display->pixBuf);
}

static void renderMainMenu(GameContext *gameCtx) {
  MenuRender(gameCtx->display->currentMenu, gameCtx,
             &gameCtx->display->defaultFont, gameCtx->display->pixBuf);
}

static void renderPlayField(GameContext *gameCtx) {
  DisplayRenderCPS(gameCtx->display, &gameCtx->display->playField,
                   PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  DisplayRenderCPSPart(gameCtx->display, &gameCtx->display->playField,
                       UI_MAP_BUTTON_X, UI_MAP_BUTTON_Y, 114, 65,
                       UI_MAP_BUTTON_W, UI_MAP_BUTTON_H, 320);
  if (gameCtx->display->controlDisabled) {
    DisplayDrawDisabledOverlay(gameCtx->display, UI_TURN_LEFT_BUTTON_X,
                               UI_TURN_LEFT_BUTTON_Y, UI_DIR_BUTTON_W * 3,
                               UI_DIR_BUTTON_H * 2);
    DisplayDrawDisabledOverlay(gameCtx->display, UI_MENU_BUTTON_X,
                               UI_MENU_BUTTON_Y, UI_MENU_INV_BUTTON_W * 2,
                               UI_MENU_INV_BUTTON_H);
  }
}

static void drawLevelBar(GameContext *gameCtx, int startX, int startY,
                         SDL_Color col) {
  UIFillRect(gameCtx->display->pixBuf, startX, startY, 5, 32, col);
}

static void renderCharFace(GameContext *gameCtx, uint8_t charId, int x) {
  SHPFrame frame = {0};
  assert(SHPHandleGetFrame(&gameCtx->display->charFaces[charId], &frame, 0));
  SHPFrameGetImageData(&frame);
  DisplayRenderSHP(gameCtx->display, &frame, x, CHAR_ZONE_Y + 1,
                   gameCtx->display->defaultPalette);
  SHPFrameRelease(&frame);
}

static void renderCharZone(GameContext *gameCtx, uint8_t charId, int x) {
  UIFillRect(gameCtx->display->pixBuf, x, CHAR_ZONE_Y, CHAR_ZONE_W, CHAR_ZONE_H,
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
  UIRenderText(&gameCtx->display->font6p, gameCtx->display->pixBuf,
               x + xManaBar, CHAR_ZONE_Y + 1, 5, c);

  UISetTextStyle(UITextStyle_Highlighted);
  GameContextGetString(gameCtx, 0X4254, c, 2);
  UIRenderText(&gameCtx->display->font6p, gameCtx->display->pixBuf,
               x + xLifeBar, CHAR_ZONE_Y + 1, 5, c);

  if (charId == gameCtx->selectedChar && gameCtx->selectedCharIsCastingSpell) {
    SHPFrame frame = {0};
    assert(SHPHandleGetFrame(&gameCtx->display->gameShapes, &frame, 73));
    SHPFrameGetImageData(&frame);
    DisplayRenderSHP(gameCtx->display, &frame, x + 44, CHAR_ZONE_Y,
                     gameCtx->display->defaultPalette);
    SHPFrameRelease(&frame);
  } else {

    {
      SHPFrame frame = {0};
      assert(SHPHandleGetFrame(&gameCtx->display->gameShapes, &frame, 54));
      SHPFrameGetImageData(&frame);
      DisplayRenderSHP(gameCtx->display, &frame, x + 44, CHAR_ZONE_Y,
                       gameCtx->display->defaultPalette);
      SHPFrameRelease(&frame);

      if (gameCtx->display->controlDisabled) {
        DisplayDrawDisabledOverlay(gameCtx->display, x + 44, CHAR_ZONE_Y, 22,
                                   18);
      }
    }
    {
      SHPFrame frame = {0};
      assert(SHPHandleGetFrame(&gameCtx->display->gameShapes, &frame, 72));
      SHPFrameGetImageData(&frame);
      DisplayRenderSHP(gameCtx->display, &frame, x + 44, CHAR_ZONE_Y + 16,
                       gameCtx->display->defaultPalette);
      SHPFrameRelease(&frame);
      if (gameCtx->display->controlDisabled) {
        DisplayDrawDisabledOverlay(gameCtx->display, x + 44, CHAR_ZONE_Y + 16,
                                   22, 18);
      }
    }
  }
  UIResetTextStyle();
  if (charId == gameCtx->selectedChar) {
    UIStrokeRect(gameCtx->display->pixBuf, x, CHAR_ZONE_Y, CHAR_ZONE_W,
                 CHAR_ZONE_H, (SDL_Color){117, 115, 145});
  }
}

static void renderLeftUIPart(GameContext *gameCtx) {
  char t[8] = "";
  UISetDefaultStyle();
  snprintf(t, 8, "%i", gameCtx->credits);
  UIRenderText(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
               UI_CREDITS_X + 5, UI_CREDITS_Y + 2, 20, t);
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



static void renderExitButton(GameContext *gameCtx) {
  UIDrawTextButton(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                   UI_SCENE_EXIT_BUTTON_X, UI_SCENE_EXIT_BUTTON_Y,
                   UI_SCENE_EXIT_BUTTON_W, UI_SCENE_EXIT_BUTTON_H, "EXIT");

  if (gameCtx->display->exitSceneButtonDisabled) {
    DisplayDrawDisabledOverlay(gameCtx->display, UI_SCENE_EXIT_BUTTON_X,
                               UI_SCENE_EXIT_BUTTON_Y, UI_SCENE_EXIT_BUTTON_W,
                               UI_SCENE_EXIT_BUTTON_H);
  }
}

void GameRender(GameContext *gameCtx) {
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

    if (gameCtx->display->showBitmap) {
      printf("show bitmap\n");
      DisplayRenderCPSAt(gameCtx->display, &gameCtx->display->loadedbitMap, 112,
                         0, 176, 120, 320, 200);
    }
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

  if (gameCtx->display->showBigDialog) {
    showBigDialogZone(gameCtx->display);
  }
  if (gameCtx->display->drawExitSceneButton) {
    renderExitButton(gameCtx);
  }

  if (gameCtx->display->buttonText[0]) {
    UIDrawTextButton(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                     DIALOG_BUTTON1_X, DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W,
                     DIALOG_BUTTON_H, gameCtx->display->buttonText[0]);
  }

  if (gameCtx->display->buttonText[1]) {
    UIDrawTextButton(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                     DIALOG_BUTTON2_X, DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W,
                     DIALOG_BUTTON_H, gameCtx->display->buttonText[1]);
  }
  if (gameCtx->display->buttonText[2]) {
    UIDrawTextButton(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                     DIALOG_BUTTON3_X, DIALOG_BUTTON_Y_2, DIALOG_BUTTON_W,
                     DIALOG_BUTTON_H, gameCtx->display->buttonText[2]);
  }
  renderDialog(gameCtx);
}
