#include "game.h"
#include "SDL_events.h"
#include "SDL_render.h"
#include "bytes.h"
#include "format_cmz.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "renderer.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int frontDist;
  int leftDist;
} ViewConeCell;

static ViewConeCell viewConeCell[] = {
    {3, -3}, // A
    {3, -2}, // B
    {3, -1}, // C
    {3, 0},  // D
    {3, 1},  // E
    {3, 2},  // F
    {3, 3},  // G

    {2, -2}, // H
    {2, -1}, // I
    {2, 0},  // J
    {2, 1},  // K
    {2, 2},  // L
    {1, 1},  // M
    {1, 0},  // N
    {1, -1}, // O
    {0, 1},  // P
    {0, -1}, // Q
};
#define VIEW_CONE_NUM_CELLS 17

typedef enum {
  CELL_A = 0,
  CELL_B,
  CELL_C,
  CELL_D,
  CELL_E,
  CELL_F,
  CELL_G,
  CELL_H,
  CELL_I,
  CELL_J,
  CELL_K,
  CELL_L,
  CELL_M,
  CELL_N,
  CELL_O,
  CELL_P,
  CELL_Q,
} CELL_ID;

/*
    Field of vision: the 17 map positions required to read for rendering a
   screen and the 25 possible wall configurations that these positions might
   contain.

    A|B|C|D|E|F|G
      ¯ ¯ ¯ ¯ ¯
      H|I|J|K|L
        ¯ ¯ ¯
        M|N|O
        ¯ ¯ ¯
        P|^|Q
*/

typedef enum {
  North = 0,
  East = 1,
  South = 2,
  West = 3,
} Orientation;

typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  Point coords;
  uint8_t valid;
} ViewConeEntry;

typedef struct {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  MazeHandle mazHandle;
  Point partyPos;
  Orientation orientation;

  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];
} LevelContext;

#define SCREEN_WIDTH 1000
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
static void GameRenderView(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                           int yOff);

static void GameRenderMap(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                          int yOff);

static int GameRun(LevelContext *ctx) {
  ctx->partyPos.x = 13;
  ctx->partyPos.y = 18;
  ctx->orientation = South;
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

static void GameRenderFrame(SDL_Renderer *renderer, LevelContext *ctx) {
  GameRenderMap(renderer, ctx, 500, 10);
  printf("--------------\n");
  for (int i = 0; i < VIEW_CONE_NUM_CELLS; i++) {
    if (ctx->viewConeEntries[i].valid == 0) {
      continue;
    }
    printf("%i %i %i\n", i, ctx->viewConeEntries[i].coords.x,
           ctx->viewConeEntries[i].coords.y);
  }
  printf("--------------\n");

  GameRenderView(renderer, ctx, 500, 100);
}

void GameRenderScene(SDL_Renderer *renderer, LevelContext *ctx, int wallType) {
  drawBackground(renderer, &ctx->vcnHandle, &ctx->vmpHandle);

  const ViewConeEntry *aEntry = ctx->viewConeEntries + CELL_A;
  if (aEntry->valid) {
    int index = aEntry->coords.y * 32 + aEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 0);
    }
  }

  const ViewConeEntry *bEntry = ctx->viewConeEntries + CELL_B;
  if (bEntry->valid) {
    int index = bEntry->coords.y * 32 + bEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 1);
    }
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 6);
    }
  }

  const ViewConeEntry *cEntry = ctx->viewConeEntries + CELL_C;
  if (cEntry->valid) {
    int index = cEntry->coords.y * 32 + cEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 2);
    }
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 7);
    }
  }

  const ViewConeEntry *dEntry = ctx->viewConeEntries + CELL_D;
  if (dEntry->valid) {
    int index = dEntry->coords.y * 32 + dEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 8);
    }
  }

  const ViewConeEntry *eEntry = ctx->viewConeEntries + CELL_E;
  if (eEntry->valid) {
    int index = eEntry->coords.y * 32 + eEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 9);
    }
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 3);
    }
  }

  const ViewConeEntry *fEntry = ctx->viewConeEntries + CELL_F;
  if (fEntry->valid) {
    int index = fEntry->coords.y * 32 + fEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 10);
    }
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 4);
    }
  }

  const ViewConeEntry *gEntry = ctx->viewConeEntries + CELL_G;
  if (gEntry->valid) {
    int index = gEntry->coords.y * 32 + gEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 5);
    }
  }

  const ViewConeEntry *hEntry = ctx->viewConeEntries + CELL_H;
  if (hEntry->valid) {
    int index = hEntry->coords.y * 32 + hEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 11);
    }
  }

  const ViewConeEntry *iEntry = ctx->viewConeEntries + CELL_I;
  if (iEntry->valid) {
    int index = iEntry->coords.y * 32 + iEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 12);
    }
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 15);
    }
  }

  const ViewConeEntry *kEntry = ctx->viewConeEntries + CELL_K;
  if (kEntry->valid) {
    int index = kEntry->coords.y * 32 + kEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 13);
    }
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 17);
    }
  }

  const ViewConeEntry *lEntry = ctx->viewConeEntries + CELL_L;
  if (lEntry->valid) {
    int index = lEntry->coords.y * 32 + lEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 14);
    }
  }

  const ViewConeEntry *jEntry = ctx->viewConeEntries + CELL_J;
  if (jEntry->valid) {
    int index = jEntry->coords.y * 32 + jEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 16);
    }
  }

  const ViewConeEntry *mEntry = ctx->viewConeEntries + CELL_M;
  if (mEntry->valid) {
    int index = mEntry->coords.y * 32 + mEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 20);
    }
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 18);
    }
  }

  const ViewConeEntry *oEntry = ctx->viewConeEntries + CELL_O;
  if (oEntry->valid) {
    int index = oEntry->coords.y * 32 + oEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 21);
    }
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 19);
    }
  }

  const ViewConeEntry *nEntry = ctx->viewConeEntries + CELL_N;
  if (nEntry->valid) {
    int index = nEntry->coords.y * 32 + nEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->south) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 22);
    }
  }

  const ViewConeEntry *pEntry = ctx->viewConeEntries + CELL_P;
  if (pEntry->valid) {
    int index = pEntry->coords.y * 32 + pEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->east) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 23);
    }
  }

  const ViewConeEntry *qEntry = ctx->viewConeEntries + CELL_Q;
  if (qEntry->valid) {
    int index = qEntry->coords.y * 32 + qEntry->coords.x;
    const MazeBlock *block = ctx->mazHandle.maze->wallMappingIndices + index;
    if (block->west) {
      drawWall(renderer, &ctx->vcnHandle, &ctx->vmpHandle, 1, 24);
    }
  }
}

static void GameRenderView(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                           int yOff) {
  GameRenderScene(renderer, ctx, 1);
}

static Point goFront(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.y -= distance;
    break;
  case East:
    pt.x += distance;
    break;
  case South:
    pt.y += distance;
    break;
  case West:
    pt.x -= distance;
    break;
  }
  return pt;
}

static Point goLeft(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.x += distance;
    break;
  case East:
    pt.y += distance;
    break;
  case South:
    pt.x -= distance;
    break;
  case West:
    pt.y -= distance;
    break;
  }
  return pt;
}

static Point goRight(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.x -= distance;
    break;
  case East:
    pt.y -= distance;
    break;
  case South:
    pt.x += distance;
    break;
  case West:
    pt.y += distance;
    break;
  }
  return pt;
}

static Point go(const Point *pos, Orientation orientation, int frontDist,
                int leftDist) {
  Point p = *pos;
  if (frontDist) {
    p = goFront(&p, orientation, frontDist);
  }
  if (leftDist > 0) {
    p = goRight(&p, orientation, leftDist);
  } else if (leftDist < 0) {
    p = goLeft(&p, orientation, -leftDist);
  }
  return p;
}

static int cellInCone(LevelContext *ctx, int x, int y) {
  for (int i = 0; i < VIEW_CONE_NUM_CELLS; i++) {
    Point p = go(&ctx->partyPos, ctx->orientation, viewConeCell[i].frontDist,
                 viewConeCell[i].leftDist);
    if (p.x >= 0 && p.x < 32 && p.y >= 0 && p.y < 32) {
      ctx->viewConeEntries[i].coords = p;
      ctx->viewConeEntries[i].valid = 1;
    }

    if (p.x == x && p.y == y) {
      return 1;
    }
  }
  return 0;
}

static void GameRenderMap(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                          int yOff) {
  const int cellSize = 12;
  SDL_Rect mapRect;
  mapRect.w = 32 * cellSize;
  mapRect.h = 32 * cellSize;
  mapRect.x = xOff;
  mapRect.y = yOff;
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
      if (x == ctx->partyPos.x && y == ctx->partyPos.y) {
        SDL_Rect posR;
        posR.w = cellSize - 12;
        posR.h = cellSize - 12;
        posR.x = 6 + xOff + x * cellSize;
        posR.y = 6 + yOff + y * cellSize;
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &posR);
        int centerX = posR.x + posR.w / 2;
        int centerY = posR.y + posR.h / 2;
        switch (ctx->orientation) {
        case North:
          SDL_RenderDrawLine(renderer, centerX, centerY, centerX,
                             centerY - cellSize / 2);
          break;
        case East:
          SDL_RenderDrawLine(renderer, centerX, centerY, centerX + cellSize / 2,
                             centerY);
          break;
        case South:
          SDL_RenderDrawLine(renderer, centerX, centerY, centerX,
                             centerY + cellSize / 2);
          break;
        case West:
          SDL_RenderDrawLine(renderer, centerX, centerY, centerX - cellSize / 2,
                             centerY);
          break;
        default:
          assert(0);
          break;
        }
      }
      if (cellInCone(ctx, x, y)) {
        // SDL_SetRenderDrawColor(renderer, 0, 0, 255, 127);
        // SDL_RenderFillRect(renderer, &cellR);
      }
    } // y loop
  } // x loop
}
