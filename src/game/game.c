#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
#include "dbg_server.h"
#include "game_callbacks.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "geometry.h"
#include "render.h"
#include "script.h"
#include "script_builtins.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int GameRun(GameContext *gameCtx);

int cmdGame(int argc, char *argv[]) {
  assert(GameEnvironmentInit("data"));

  if (argc < 2) {
    printf("game chapterID levelID\n");
    return 0;
  }

  int chapterId = atoi(argv[0]);
  int levelId = atoi(argv[1]);
  if (chapterId < 1) {
    return 1;
  }
  if (levelId < 1) {
    return 1;
  }
  printf("Loading chapter %i level %i\n", chapterId, levelId);

  assert(GameEnvironmentLoadChapter(chapterId));

  GameContext gameCtx = {0};
  if (!GameContextInit(&gameCtx)) {
    return 1;
  }

  GameContextInstallCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;

  LevelContext levelCtx = {0};
  gameCtx.level = &levelCtx;
  GameContextLoadLevel(&gameCtx, levelId, 0X24D, North);

  GameRun(&gameCtx);
  LevelContextRelease(&levelCtx);
  GameContextRelease(&gameCtx);

  printf("GameEnvironmentRelease\n");
  GameEnvironmentRelease();
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static void clickOnFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  GameContextRunScript(gameCtx, nextBlock);
}

static void moveFront(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->y -= 1;
    break;
  case East:
    pt->x += 1;
    break;
  case South:
    pt->y += 1;
    break;
  case West:
    pt->x -= 1;
    break;
  }
}

static void moveBack(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->y += 1;
    break;
  case East:
    pt->x -= 1;
    break;
  case South:
    pt->y -= 1;
    break;
  case West:
    pt->x += 1;
    break;
  }
}

static void moveLeft(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->x -= 1;
    break;
  case East:
    pt->y -= 1;
    break;
  case South:
    pt->x += 1;
    break;
  case West:
    pt->y += 1;
    break;
  }
}

static void moveRight(Point *pt, Orientation orientation) {
  switch (orientation) {
  case North:
    pt->x += 1;
    break;
  case East:
    pt->y += 1;
    break;
  case South:
    pt->x -= 1;
    break;
  case West:
    pt->y -= 1;
    break;
  }
}
static Orientation turnLeft(Orientation orientation) {
  orientation -= 1;
  if ((int)orientation < 0) {
    return West;
  }
  return orientation;
}

static Orientation turnRight(Orientation orientation) {
  orientation += 1;
  if (orientation > West) {
    return North;
  }
  return orientation;
}

static int processMouse(GameContext *gameCtx) {
  if (gameCtx->mouseEv.pos.x >= UI_TURN_LEFT_BUTTON_X &&
      gameCtx->mouseEv.pos.y >= UI_TURN_LEFT_BUTTON_Y &&
      gameCtx->mouseEv.pos.x < (UI_TURN_LEFT_BUTTON_X + (UI_BUTTON_W * 3)) &&
      gameCtx->mouseEv.pos.y < (UI_TURN_LEFT_BUTTON_Y + (UI_BUTTON_W * 2))) {
    int x = gameCtx->mouseEv.pos.x - UI_TURN_LEFT_BUTTON_X;
    int y = gameCtx->mouseEv.pos.y - UI_TURN_LEFT_BUTTON_Y;
    int buttonX = (int)(x / UI_BUTTON_W);
    int buttonY = (int)(y / UI_BUTTON_W);
    if (buttonX <= 2 && buttonY <= 1) {
      if (buttonY == 0) {
        if (buttonX == 0) {
          gameCtx->orientation = turnLeft(gameCtx->orientation);
        } else if (buttonX == 1) {
          moveFront(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 2) {
          gameCtx->orientation = turnRight(gameCtx->orientation);
        }
      } else if (buttonY == 1) {
        if (buttonX == 0) {
          moveLeft(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 1) {
          moveBack(&gameCtx->partyPos, gameCtx->orientation);
        } else if (buttonX == 2) {
          moveRight(&gameCtx->partyPos, gameCtx->orientation);
        }
      }
      return 1;
    }
  } else if (gameCtx->mouseEv.pos.x >= UI_MENU_BUTTON_X &&
             gameCtx->mouseEv.pos.y >= UI_MENU_BUTTON_Y &&
             gameCtx->mouseEv.pos.x < (UI_MENU_BUTTON_X + (UI_BUTTON_W * 2)) &&
             gameCtx->mouseEv.pos.y < (UI_MENU_BUTTON_Y + (UI_BUTTON_W * 1))) {
    int x = gameCtx->mouseEv.pos.x - UI_MENU_BUTTON_X;
    int buttonX = (int)(x / UI_BUTTON_W);
    if (buttonX == 0) {
      printf("Menu button\n");
    } else if (buttonX == 1) {
      printf("sleep button\n");
    }
  } else if (gameCtx->mouseEv.pos.x >= UI_INVENTORY_BUTTON_X &&
             gameCtx->mouseEv.pos.y >= UI_INVENTORY_BUTTON_Y &&
             gameCtx->mouseEv.pos.x <
                 (UI_INVENTORY_BUTTON_X + (UI_BUTTON_W * 11)) &&
             gameCtx->mouseEv.pos.y <
                 (UI_INVENTORY_BUTTON_Y + (UI_BUTTON_W * 1))) {
    int x = gameCtx->mouseEv.pos.x - UI_INVENTORY_BUTTON_X;
    int buttonX = (int)(x / UI_BUTTON_W);
    // 0 is left arrow, 10 is right arrow
    if (buttonX == 0) {
      printf("Inventory button Left\n");
    } else if (buttonX == 10) {
      printf("Inventory button Right\n");
    } else {
      printf("Inventory button %i\n", buttonX);
    }
  } else if (gameCtx->mouseEv.pos.x >= MAZE_COORDS_X &&
             gameCtx->mouseEv.pos.y >= MAZE_COORDS_Y &&
             (gameCtx->mouseEv.pos.x < (MAZE_COORDS_X + MAZE_COORDS_W)) &&
             (gameCtx->mouseEv.pos.y < (MAZE_COORDS_Y + MAZE_COORDS_H))) {
    int x = gameCtx->mouseEv.pos.x - MAZE_COORDS_X;
    int y = gameCtx->mouseEv.pos.y - MAZE_COORDS_Y;
    printf("maze click %i %i\n", x, y);
    clickOnFrontWall(gameCtx);
    return 1;
  } else {
    printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
  }
  return 0;
}

static int processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  int shouldUpdate = 0;
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    assert(gameCtx->mouseEv.pending == 0); // prev one needs to be handled
    gameCtx->mouseEv.pending = 1;
    gameCtx->mouseEv.pos.x = e->motion.x / SCREEN_FACTOR;
    gameCtx->mouseEv.pos.y = e->motion.y / SCREEN_FACTOR;

  } else if (e->type == SDL_KEYDOWN) {
    switch (e->key.keysym.sym) {
    case SDLK_z:
      // go front
      shouldUpdate = 1;
      moveFront(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_s:
      // go back
      shouldUpdate = 1;
      moveBack(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_q:
      // go left
      shouldUpdate = 1;
      moveLeft(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_d:
      // go right
      shouldUpdate = 1;
      moveRight(&gameCtx->partyPos, gameCtx->orientation);
      break;
    case SDLK_a:
      // turn anti-clockwise
      shouldUpdate = 1;
      gameCtx->orientation = turnLeft(gameCtx->orientation);

      break;
    case SDLK_e:
      // turn clockwise
      shouldUpdate = 1;
      gameCtx->orientation = turnRight(gameCtx->orientation);
      break;
    case SDLK_SPACE:
      shouldUpdate = 1;
      clickOnFrontWall(gameCtx);
      break;
    default:
      break;
    }
  }
  return shouldUpdate;
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

static void GameRender(GameContext *gameCtx) {
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gameCtx->renderer);
  renderBackground(gameCtx);

  if (gameCtx->fadeOutFrames) {

    gameCtx->fadeOutFrames--;
    clearMazeZone(gameCtx);

  } else {
    GameRenderScene(gameCtx);

    if (gameCtx->state == GameState_TimAnimation) {
      if (GameTimAnimatorRender(&gameCtx->timAnimator) == 0) {
        GameContextSetState(gameCtx, GameState_PlayGame);
      }
    }
  }

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

static int GameRun(GameContext *gameCtx) {
  int quit = 0;
  int shouldUpdate = 1;
  int firstTime = 1;
  // Event loop
  while (!quit) {
    DBGServerUpdate(gameCtx);
    while (gameCtx->state == GameState_PlayGame &&
           EMCInterpreterIsValid(&gameCtx->interp, &gameCtx->interpState)) {
      if (EMCInterpreterRun(&gameCtx->interp, &gameCtx->interpState) == 0) {
        printf("EMCInterpreterRun returned 0\n");
      }
      shouldUpdate = 1;
    }

    SDL_Event e;
    SDL_WaitEventTimeout(&e, 20);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
    if (gameCtx->state == GameState_PlayGame) {
      shouldUpdate = processGameInputs(gameCtx, &e);
      if (shouldUpdate) {
        uint16_t gameX = 0;
        uint16_t gameY = 0;
        GetGameCoords(gameCtx->partyPos.x, gameCtx->partyPos.y, &gameX, &gameY);
        gameCtx->currentBock = BlockFromCoords(gameX, gameY);
      }
    } else if (gameCtx->state == GameState_TimAnimation) {
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
        shouldUpdate = 1;
      }
    }
    if (firstTime) {
      shouldUpdate = 1;
      firstTime = 0;
    }
    if (gameCtx->mouseEv.pending) {
      shouldUpdate = processMouse(gameCtx);
      gameCtx->mouseEv.pending = 0;
    }
    if (shouldUpdate) {
      GameRender(gameCtx);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
}
