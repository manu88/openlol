#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_render.h"
#include "console.h"
#include "game_callbacks.h"
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
  case SDLK_ESCAPE:
    gameCtx->consoleHasFocus = 1;
    printf("set console focus\n");
    SDL_StartTextInput();
    break;
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
  memset(gameCtx->level->viewConeEntries, 0,
         sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
  SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
  SDL_RenderClear(gameCtx->renderer);
  GameRenderFrame(gameCtx);
  renderTextStats(gameCtx);
  renderDialog(gameCtx);
  SDL_RenderPresent(gameCtx->renderer);
}

static int GameRun(GameContext *gameCtx) {
  int quit = 0;
  int shouldUpdate = 1;

  // Event loop
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
    if (gameCtx->consoleHasFocus) {
      shouldUpdate = processConsoleInputs(gameCtx, &e);
    } else {
      if (e.type == SDL_KEYDOWN) {
        shouldUpdate = processGameInputs(gameCtx, &e);
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
