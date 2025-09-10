#include "game.h"
#include "SDL_render.h"
#include "bytes.h"
#include "format_cmz.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  MazeHandle mazHandle;
} LevelContext;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

static int GameRun(LevelContext *ctx);

int cmdGame(int argc, char *argv[]) {
  if (argc < 3) {
    printf("game maz-file vcn-file vmp-file wallpos\n");
    return 0;
  }
  const char *mazFile = argv[0];
  const char *vcnFile = argv[1];
  const char *vmpFile = argv[2];

  printf("maz='%s' vcn='%s' vmp='%s'\n", mazFile, vcnFile, vmpFile);

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
  printf("Got all files\n");
  GameRun(&levelCtx);
  VMPHandleRelease(&levelCtx.vmpHandle);
  VCNHandleRelease(&levelCtx.vcnHandle);
  MazeHandleRelease(&levelCtx.mazHandle);
  return 0;
}
static void GameRenderFrame(SDL_Renderer *renderer, LevelContext *ctx);
static void GameRenderMap(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                          int yOff);

static int GameRun(LevelContext *ctx) {
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

  // Event loop
  while (!quit) {
    SDL_Event e;

    // Wait indefinitely for the next available event
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
    GameRenderFrame(renderer, ctx);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

static void GameRenderFrame(SDL_Renderer *renderer, LevelContext *ctx) {
  GameRenderMap(renderer, ctx, 100, 100);
}

static void GameRenderMap(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                          int yOff) {
  const int cellSize = 20;
  SDL_Rect mapRect;
  mapRect.w = 32 * cellSize;
  mapRect.h = 32 * cellSize;
  mapRect.x = xOff;
  mapRect.y = yOff;
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &mapRect);
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 32; y++) {
      MazeBlock block = ctx->mazHandle.maze->wallMappingIndices[y * 32 + x];
      SDL_Rect cellR;
      cellR.w = cellSize - 2;
      cellR.h = cellSize - 2;
      cellR.x = xOff + x * cellSize;
      cellR.y = yOff + y * cellSize;
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

      if (block.north) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y, cellR.x + cellR.w,
                           cellR.y);
      }
      if (block.south) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y + cellR.h,
                           cellR.x + cellR.w, cellR.y + cellR.h);
      }
      if (block.west) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y, cellR.x,
                           cellR.y + cellR.h);
      }
      if (block.east) {
        SDL_RenderDrawLine(renderer, cellR.x + cellR.w, cellR.y,
                           cellR.x + cellR.w, cellR.y + cellR.h);
      }
      // SDL_RenderDrawRect(renderer, &cellR);
    }
  }
}
