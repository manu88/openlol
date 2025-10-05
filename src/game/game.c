#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_rect.h"
#include "SDL_render.h"
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

static int processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  int shouldUpdate = 1;
  switch (e->key.keysym.sym) {
  case SDLK_z:
    // go front
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.y -= 1;
      break;
    case East:
      gameCtx->partyPos.x += 1;
      break;
    case South:
      gameCtx->partyPos.y += 1;
      break;
    case West:
      gameCtx->partyPos.x -= 1;
      break;
    }
    break;
  case SDLK_s:
    // go back
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.y += 1;
      break;
    case East:
      gameCtx->partyPos.x -= 1;
      break;
    case South:
      gameCtx->partyPos.y -= 1;
      break;
    case West:
      gameCtx->partyPos.x += 1;
      break;
    }
    break;
  case SDLK_q:
    // go left
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.x -= 1;
      break;
    case East:
      gameCtx->partyPos.y -= 1;
      break;
    case South:
      gameCtx->partyPos.x += 1;
      break;
    case West:
      gameCtx->partyPos.y += 1;
      break;
    }
    break;
  case SDLK_d:
    // go right
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.x += 1;
      break;
    case East:
      gameCtx->partyPos.y += 1;
      break;
    case South:
      gameCtx->partyPos.x -= 1;
      break;
    case West:
      gameCtx->partyPos.y -= 1;
      break;
    }
    break;
  case SDLK_a:
    // turn anti-clockwise
    shouldUpdate = 1;
    gameCtx->orientation -= 1;
    if ((int)gameCtx->orientation < 0) {
      gameCtx->orientation = West;
    }
    break;
  case SDLK_e:
    // turn clockwise
    shouldUpdate = 1;
    gameCtx->orientation += 1;
    if (gameCtx->orientation > West) {
      gameCtx->orientation = North;
    }
    break;
  case SDLK_SPACE:
    clickOnFrontWall(gameCtx);
    break;
  default:
    break;
  }
  return shouldUpdate;
}

static void GameRender(GameContext *gameCtx) {
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gameCtx->renderer);
  renderBackground(gameCtx);

  switch (gameCtx->state) {
  case GameState_PlayGame:
    GameRenderScene(gameCtx);
    break;
  case GameState_TimAnimation:
    if (GameTimAnimatorRender(&gameCtx->timAnimator) == 0) {
      GameContextSetState(gameCtx, GameState_PlayGame);
    }
    break;
  }
  renderDialog(gameCtx);
  // GameRenderMap(gameCtx, 640, 350);

  SDL_Rect dest = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
  assert(SDL_RenderCopy(gameCtx->renderer, gameCtx->pixBuf, NULL, &dest) == 0);
  SDL_RenderPresent(gameCtx->renderer);
}

static int GameRun(GameContext *gameCtx) {
  int quit = 0;
  int shouldUpdate = 1;

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
      if (e.type == SDL_KEYDOWN) {
        shouldUpdate = processGameInputs(gameCtx, &e);
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
    if (shouldUpdate) {
      GameRender(gameCtx);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
}
