#include "render.h"
#include "SDL_render.h"
#include "game_ctx.h"
#include "geometry.h"
#include "renderer.h"
#include <assert.h>
#include <stdint.h>

void renderCPSAt(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
                 const uint8_t *paletteBuffer, int xOff, int yOff, int w,
                 int h) {
  void *data;
  int pitch;
  SDL_LockTexture(pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      int offset = ((xOff + w) * y) + x;
      if (offset >= dataSize) {
        printf("Offset %i >= %zu\n", offset, dataSize);
      }
      assert(offset < dataSize);
      uint8_t paletteIdx = *(imgData + offset);
      uint8_t r;
      uint8_t g;
      uint8_t b;
      if (paletteBuffer) {
        r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
        g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
        b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      } else {
        r = paletteIdx;
        g = paletteIdx;
        b = paletteIdx;
      }

      drawPix(data, pitch, r, g, b, xOff + x, yOff + y);
    }
  }
  SDL_UnlockTexture(pixBuf);
}

void renderCPS(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
               const uint8_t *paletteBuffer, int w, int h) {
  void *data;
  int pitch;
  SDL_LockTexture(pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      int offset = (w * y) + x;
      if (offset >= dataSize) {
        printf("Offset %i >= %zu\n", offset, dataSize);
      }
      assert(offset < dataSize);
      uint8_t paletteIdx = *(imgData + offset);
      uint8_t r;
      uint8_t g;
      uint8_t b;
      if (paletteBuffer) {
        r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
        g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
        b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      } else {
        r = paletteIdx;
        g = paletteIdx;
        b = paletteIdx;
      }

      drawPix(data, pitch, r, g, b, x, y);
    }
  }
  SDL_UnlockTexture(pixBuf);
}

void clearMazeZone(GameContext *gameCtx) {
  void *data;
  int pitch;
  SDL_LockTexture(gameCtx->pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < MAZE_COORDS_W; x++) {
    for (int y = 0; y < MAZE_COORDS_H; y++) {
      drawPix(data, pitch, 0, 0, 0, MAZE_COORDS_X + x, MAZE_COORDS_Y + y);
    }
  }
  SDL_UnlockTexture(gameCtx->pixBuf);
}

void renderPlayField(GameContext *gameCtx) {
  renderCPS(gameCtx->pixBuf, gameCtx->playField.data,
            gameCtx->playField.imageSize, gameCtx->playField.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  clearMazeZone(gameCtx);
}

typedef struct {
  int frontDist;
  int leftDist;
} ViewConeCell;

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

static ViewConeCell viewConeCell[] = {
    {3, 3},  // A
    {3, 2},  // B
    {3, 1},  // C
    {3, 0},  // D
    {3, -1}, // E
    {3, -2}, // F
    {3, -3}, // G

    {2, 2},  // H
    {2, 1},  // I
    {2, 0},  // J
    {2, -1}, // K
    {2, -2}, // L

    {1, 1},  // M
    {1, 0},  // N
    {1, -1}, // O

    {0, 1},  // P
    {0, -1}, // Q
};

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

Orientation absOrientation(Orientation partyOrientation,
                           Orientation orientation) {
  return (orientation + partyOrientation) % 4;
}

/*

 9 7 3 7 9
 8 6 2 6 8
 8 5 1 5 8
   4 0 4
     ^=party pos.
*/

typedef enum {
  DecorationIndex_N_SOUTH = 1,
  DecorationIndex_J_SOUTH = 2,
  DecorationIndex_D_SOUTH = 3,

  DecorationIndex_P_EAST = 4,
  DecorationIndex_Q_WEST = 4,

  DecorationIndex_M_WEST = 5,
  DecorationIndex_O_EAST = 5,

  DecorationIndex_M_SOUTH = 8,
  DecorationIndex_O_SOUTH = 8,
} DecorationIndex;

static void renderDecoration(SDL_Texture *pixBuf, LevelContext *ctx,
                             DecorationIndex decorationIndex,
                             uint16_t decorationId, int destXOffset,
                             int destYOffset, uint8_t xFlip) {

  const DatDecoration *deco = ctx->datHandle.datDecoration + decorationId;

  if (deco->shapeIndex[decorationIndex] == DECORATION_EMPTY_INDEX) {
    return;
  }

  SHPFrame frame = {0};
  SHPHandleGetFrame(&ctx->shpHandle, &frame, deco->shapeIndex[decorationIndex]);
  SHPFrameGetImageData(&frame);
  drawSHPMazeFrame(pixBuf, &frame, deco->shapeX[decorationIndex] + destXOffset,
                   deco->shapeY[decorationIndex] + destYOffset,
                   ctx->vcnHandle.palette, xFlip);
  if (deco->flags & DatDecorationFlags_Mirror) {
    // only for south walls
    if (decorationIndex == DecorationIndex_N_SOUTH ||
        decorationIndex == DecorationIndex_J_SOUTH ||
        decorationIndex == DecorationIndex_D_SOUTH)
      drawSHPMazeFrame(pixBuf, &frame,
                       deco->shapeX[decorationIndex] + destXOffset,
                       deco->shapeY[decorationIndex] + destYOffset,
                       ctx->vcnHandle.palette, 1);
  }

  if (deco->next) {
    renderDecoration(pixBuf, ctx, decorationIndex, deco->next, destXOffset,
                     destYOffset, xFlip);
  }
}

static void renderWallDecoration(SDL_Texture *pixBuf, LevelContext *ctx,
                                 DecorationIndex DecorationIndex, uint8_t wmi,
                                 int destXOffset, int destYOffset,
                                 uint8_t xFlip) {
  const WllWallMapping *mapping = WllHandleGetWallMapping(&ctx->wllHandle, wmi);
  if (mapping && mapping->decorationId != 0 &&
      mapping->decorationId <= ctx->datHandle.nbrDecorations) {
    renderDecoration(pixBuf, ctx, DecorationIndex, mapping->decorationId,
                     destXOffset, destYOffset, xFlip);
  }
}

static void computeViewConeCells(GameContext *gameCtx, int x, int y) {
  memset(gameCtx->viewConeEntries, 0,
         sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
  for (int i = 0; i < VIEW_CONE_NUM_CELLS; i++) {
    Point p = PointGo(&gameCtx->partyPos, gameCtx->orientation,
                      viewConeCell[i].frontDist, viewConeCell[i].leftDist);
    if (p.x >= 0 && p.x < 32 && p.y >= 0 && p.y < 32) {
      gameCtx->viewConeEntries[i].coords = p;
      gameCtx->viewConeEntries[i].valid = 1;
    }
  }
}

void GameRenderMap(GameContext *gameCtx, int xOff, int yOff) {
  SDL_Renderer *renderer = gameCtx->renderer;
  const int cellSize = 12;
  SDL_Rect mapRect;
  mapRect.w = 32 * cellSize;
  mapRect.h = 32 * cellSize;
  mapRect.x = xOff;
  mapRect.y = yOff;
  SDL_RenderDrawRect(renderer, &mapRect);

  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 32; y++) {
      MazeBlock block =
          gameCtx->level->mazHandle.maze->wallMappingIndices[y * 32 + x];
      SDL_Rect cellR;
      cellR.w = cellSize - 2;
      cellR.h = cellSize - 2;
      cellR.x = xOff + x * cellSize;
      cellR.y = yOff + y * cellSize;
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      if (block.face[North]) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y, cellR.x + cellR.w,
                           cellR.y);
      }
      if (block.face[South]) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y + cellR.h,
                           cellR.x + cellR.w, cellR.y + cellR.h);
      }
      if (block.face[West]) {
        SDL_RenderDrawLine(renderer, cellR.x, cellR.y, cellR.x,
                           cellR.y + cellR.h);
      }
      if (block.face[East]) {
        SDL_RenderDrawLine(renderer, cellR.x + cellR.w, cellR.y,
                           cellR.x + cellR.w, cellR.y + cellR.h);
      }
      if (x == gameCtx->partyPos.x && y == gameCtx->partyPos.y) {
        SDL_Rect posR;
        posR.w = cellSize - 12;
        posR.h = cellSize - 12;
        posR.x = 6 + xOff + x * cellSize;
        posR.y = 6 + yOff + y * cellSize;
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &posR);
        int centerX = posR.x + posR.w / 2;
        int centerY = posR.y + posR.h / 2;
        switch (gameCtx->orientation) {
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
    } // y loop
  } // x loop
}

void GameRenderScene(GameContext *gameCtx) {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 32; y++) {
      computeViewConeCells(gameCtx, x, y);
    }
  }
  LevelContext *level = gameCtx->level;
  drawCeilingAndFloor(gameCtx->pixBuf, &level->vcnHandle, &level->vmpHandle);

  const ViewConeEntry *aEntry = gameCtx->viewConeEntries + CELL_A;
  const ViewConeEntry *bEntry = gameCtx->viewConeEntries + CELL_B;
  const ViewConeEntry *cEntry = gameCtx->viewConeEntries + CELL_C;
  const ViewConeEntry *dEntry = gameCtx->viewConeEntries + CELL_D;
  const ViewConeEntry *eEntry = gameCtx->viewConeEntries + CELL_E;
  const ViewConeEntry *fEntry = gameCtx->viewConeEntries + CELL_F;
  const ViewConeEntry *gEntry = gameCtx->viewConeEntries + CELL_G;
  const ViewConeEntry *hEntry = gameCtx->viewConeEntries + CELL_H;
  const ViewConeEntry *iEntry = gameCtx->viewConeEntries + CELL_I;
  const ViewConeEntry *jEntry = gameCtx->viewConeEntries + CELL_J;
  const ViewConeEntry *kEntry = gameCtx->viewConeEntries + CELL_K;
  const ViewConeEntry *lEntry = gameCtx->viewConeEntries + CELL_L;
  const ViewConeEntry *mEntry = gameCtx->viewConeEntries + CELL_M;
  const ViewConeEntry *nEntry = gameCtx->viewConeEntries + CELL_N;
  const ViewConeEntry *oEntry = gameCtx->viewConeEntries + CELL_O;
  const ViewConeEntry *pEntry = gameCtx->viewConeEntries + CELL_P;
  const ViewConeEntry *qEntry = gameCtx->viewConeEntries + CELL_Q;

  if (aEntry->valid) {
    int index = aEntry->coords.y * 32 + aEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 A_east);
      }
    }
  }

  if (bEntry->valid) {
    int index = bEntry->coords.y * 32 + bEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 B_east);
      }
    }
  }
  if (cEntry->valid) {
    int index = cEntry->coords.y * 32 + cEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 C_east);
      }
    }
  }
  if (eEntry->valid) {
    int index = eEntry->coords.y * 32 + eEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 E_west);
      }
    }
  }
  if (fEntry->valid) {
    int index = fEntry->coords.y * 32 + fEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 F_west);
      }
    }
  }

  if (gEntry->valid) {
    int index = gEntry->coords.y * 32 + gEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 G_west);
      }
    }
  }

  if (bEntry->valid) {
    int index = bEntry->coords.y * 32 + bEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 B_south);
      }
    }
  }

  if (cEntry->valid) {
    int index = cEntry->coords.y * 32 + cEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 C_south);
      }
    }
  }

  if (dEntry->valid) {
    int index = dEntry->coords.y * 32 + dEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 D_south);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_D_SOUTH, wmi,
                           64, 26, 0);
    }
  }

  if (eEntry->valid) {
    int index = eEntry->coords.y * 32 + eEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 E_south);
      }
    }
  }

  if (fEntry->valid) {
    int index = fEntry->coords.y * 32 + fEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 F_south);
      }
    }
  }

  if (hEntry->valid) {
    int index = hEntry->coords.y * 32 + hEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 H_east);
      }
    }
  }
  if (iEntry->valid) {
    int index = iEntry->coords.y * 32 + iEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 I_east);
      }
    }
  }

  if (kEntry->valid) {
    int index = kEntry->coords.y * 32 + kEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 K_west);
      }
    }
  }
  if (lEntry->valid) {
    int index = lEntry->coords.y * 32 + lEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 L_west);
      }
    }
  }

  if (iEntry->valid) {
    int index = iEntry->coords.y * 32 + iEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 I_south);
      }
    }
  }

  if (jEntry->valid) {
    int index = jEntry->coords.y * 32 + jEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 J_south);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_J_SOUTH, wmi,
                           48, 20, 0);
    }
  }

  if (kEntry->valid) {
    int index = kEntry->coords.y * 32 + kEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 K_south);
      }
    }
  }

  if (mEntry->valid) {
    int index = mEntry->coords.y * 32 + mEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 M_east);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_M_WEST, wmi,
                           24, 10, 0);
    }
  }

  if (oEntry->valid) {
    int index = oEntry->coords.y * 32 + oEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 O_west);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_O_EAST, wmi,
                           24, 10, 1);
    }
  }

  if (mEntry->valid) {
    int index = mEntry->coords.y * 32 + mEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 M_south);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_M_SOUTH, wmi,
                           0, 0, 0);
    }
  }

  if (oEntry->valid) {
    int index = oEntry->coords.y * 32 + oEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 O_south);
      }
      // renderWallDecoration(renderer, ctx, DecorationIndex_O_SOUTH, wmi, 24,
      // 10,
      //                      1);
    }
  }

  // front facing wall
  if (nEntry->valid) {
    int index = nEntry->coords.y * 32 + nEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, South)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 N_south);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_N_SOUTH, wmi,
                           24, 8, 0);
    }
  }

  if (pEntry->valid) {
    int index = pEntry->coords.y * 32 + pEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, East)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 P_east);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_P_EAST, wmi,
                           0, 0, 0);
    }
  }

  if (qEntry->valid) {
    int index = qEntry->coords.y * 32 + qEntry->coords.x;
    const MazeBlock *block = level->mazHandle.maze->wallMappingIndices + index;
    uint8_t wmi = block->face[absOrientation(gameCtx->orientation, West)];
    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {
        drawWall(gameCtx, &level->vcnHandle, &level->vmpHandle, wallType,
                 Q_west);
      }
      renderWallDecoration(gameCtx->pixBuf, level, DecorationIndex_Q_WEST, wmi,
                           0, 0, 1);
    }
  }
}
