#include "game.h"
#include "SDL_events.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "bytes.h"
#include "format_cmz.h"
#include "format_dat.h"
#include "format_shp.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include "geometry.h"
#include "render.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale/_stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

static int GameRun(GameContext *gameCtx, LevelContext *ctx);
static int GameInit(GameContext *gameCtx);

int cmdGame(int argc, char *argv[]) {
  if (argc < 5) {
    printf("game maz-file vcn-file vmp-file wall-file dat-file shp-file\n");
    return 0;
  }
  const char *mazFile = argv[0];
  const char *vcnFile = argv[1];
  const char *vmpFile = argv[2];
  const char *wllFile = argv[3];

  const char *datFile = argv[4];
  const char *shpFile = argv[5];

  printf("maz='%s' vcn='%s' vmp='%s' wll='%s'\n", mazFile, vcnFile, vmpFile,
         wllFile);

  LevelContext levelCtx = {0};
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vcnFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VCNHandleFromLCWBuffer(&levelCtx.vcnHandle, buffer, fileSize)) {
      printf("VCNDataFromLCWBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vmpFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VMPHandleFromLCWBuffer(&levelCtx.vmpHandle, buffer, fileSize)) {
      printf("VMPDataFromLCWBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(mazFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!MazeHandleFromBuffer(&levelCtx.mazHandle, buffer, fileSize)) {
      printf("MazeHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(wllFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!WllHandleFromBuffer(&levelCtx.wllHandle, buffer, fileSize)) {
      printf("WllHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(datFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!DatHandleFromBuffer(&levelCtx.datHandle, buffer, fileSize)) {
      printf("DatHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(shpFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!SHPHandleFromBuffer(&levelCtx.shpHandle, buffer, fileSize)) {
      printf("SHPHandleFromBuffer error\n");
      return 1;
    }
  }
  printf("Got all files\n");

  GameContext gameCtx = {0};
  if (!GameInit(&gameCtx)) {
    return 1;
  }
  GameRun(&gameCtx, &levelCtx);
  LevelContextRelease(&levelCtx);
  GameContextRelease(&gameCtx);
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static int processGameInputs(LevelContext *ctx, const SDL_Event *e) {
  int shouldUpdate = 1;
  switch (e->key.keysym.sym) {
  case SDLK_z:
    // go front
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.y -= 1;
      break;
    case East:
      ctx->partyPos.x += 1;
      break;
    case South:
      ctx->partyPos.y += 1;
      break;
    case West:
      ctx->partyPos.x -= 1;
      break;
    }
    break;
  case SDLK_s:
    // go back
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.y += 1;
      break;
    case East:
      ctx->partyPos.x -= 1;
      break;
    case South:
      ctx->partyPos.y -= 1;
      break;
    case West:
      ctx->partyPos.x += 1;
      break;
    }
    break;
  case SDLK_q:
    // go left
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.x -= 1;
      break;
    case East:
      ctx->partyPos.y -= 1;
      break;
    case South:
      ctx->partyPos.x += 1;
      break;
    case West:
      ctx->partyPos.y += 1;
      break;
    }
    break;
  case SDLK_d:
    // go right
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.x += 1;
      break;
    case East:
      ctx->partyPos.y += 1;
      break;
    case South:
      ctx->partyPos.x -= 1;
      break;
    case West:
      ctx->partyPos.y -= 1;
      break;
    }
    break;
  case SDLK_a:
    // turn anti-clockwise
    shouldUpdate = 1;
    ctx->orientation -= 1;
    if ((int)ctx->orientation < 0) {
      ctx->orientation = West;
    }
    break;
  case SDLK_e:
    // turn clockwise
    shouldUpdate = 1;
    ctx->orientation += 1;
    if (ctx->orientation > West) {
      ctx->orientation = North;
    }
    break;
  default:
    break;
  }
  return shouldUpdate;
}

static int GameInit(GameContext *gameCtx) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }
  if (TTF_Init() == -1) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }

  gameCtx->font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 18);
  if (!gameCtx->font) {
    printf("unable to create font\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }

  gameCtx->window = SDL_CreateWindow("Lands Of Lore", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                     SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!gameCtx->window) {
    printf("Window could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  gameCtx->renderer =
      SDL_CreateRenderer(gameCtx->window, -1, SDL_RENDERER_ACCELERATED);
  if (!gameCtx->renderer) {
    printf("Renderer could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  return 1;
}

static void renderStatLine(GameContext *gameCtx, const char *line, int x,
                           int y) {
  gameCtx->textSurface = TTF_RenderUTF8_Solid(gameCtx->font, line,
                                              (SDL_Color){255, 255, 255, 255});

  gameCtx->textTexture =
      SDL_CreateTextureFromSurface(gameCtx->renderer, gameCtx->textSurface);
  SDL_Rect dstrect = {x, y, gameCtx->textSurface->w, gameCtx->textSurface->h};
  SDL_RenderCopy(gameCtx->renderer, gameCtx->textTexture, NULL, &dstrect);
  SDL_DestroyTexture(gameCtx->textTexture);
  SDL_FreeSurface(gameCtx->textSurface);
}

static char textStatsBuffer[512];

static void renderTextStats(GameContext *gameCtx, LevelContext *ctx) {
  int statsPosX = 20;
  int statsPosY = 300;

  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "stats:");
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "pose x=%i y=%i o=%i %c",
           ctx->partyPos.x, ctx->partyPos.y, ctx->orientation,
           ctx->orientation == North ? 'N'
                                     : (ctx->orientation == South  ? 'S'
                                        : ctx->orientation == East ? 'E'
                                                                   : 'W'));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY + 20);
}

static int GameRun(GameContext *gameCtx, LevelContext *ctx) {
  ctx->partyPos.x = 13;
  ctx->partyPos.y = 18;
  ctx->orientation = North;

  int quit = 0;
  int shouldUpdate = 1;
  // Event loop
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      shouldUpdate = processGameInputs(ctx, &e);
    }
    if (shouldUpdate) {
      memset(ctx->viewConeEntries, 0,
             sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
      SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
      SDL_RenderClear(gameCtx->renderer);
      GameRenderFrame(gameCtx->renderer, ctx);

      renderTextStats(gameCtx, ctx);

      SDL_RenderPresent(gameCtx->renderer);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
}

void GameContextRelease(GameContext *gameCtx) {
  SDL_DestroyRenderer(gameCtx->renderer);
  SDL_DestroyWindow(gameCtx->window);
  if (gameCtx->font) {
    TTF_CloseFont(gameCtx->font);
  }
}
