#include "game.h"

#include "bytes.h"
#include "format_cmz.h"
#include "format_dat.h"
#include "format_shp.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include "render.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

static int GameRun(LevelContext *ctx);

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
  GameRun(&levelCtx);
  LevelContextRelease(&levelCtx);
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static int GameRun(LevelContext *ctx) {
  ctx->partyPos.x = 13;
  ctx->partyPos.y = 18;
  ctx->orientation = North;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  // Create window
  SDL_Window *window = SDL_CreateWindow(
      "Basic C SDL project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  int quit = 0;
  int shouldUpdate = 1;
  // Event loop
  while (!quit) {
    SDL_Event e;

    // Wait indefinitely for the next available event
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
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
    }
    if (shouldUpdate) {
      memset(ctx->viewConeEntries, 0,
             sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderClear(renderer);
      GameRenderFrame(renderer, ctx);
      SDL_RenderPresent(renderer);
      shouldUpdate = 0;
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
