#include "render.h"
#include "SDL_mouse.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "formats/format_shp.h"
#include "game_ctx.h"
#include "geometry.h"
#include "level.h"
#include "renderer.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

void createCursorForItem(GameContext *ctx, uint16_t frameId) {
  SDL_Cursor *prevCursor = ctx->renderer->cursor;

  const int w = 20 * SCREEN_FACTOR;
  SDL_Surface *s = SDL_CreateRGBSurface(0, w, w, 32, 0, 0, 0, 0);
  assert(s);
  SDL_Renderer *r = SDL_CreateSoftwareRenderer(s);
  assert(r);
  SHPFrame frame = {0};
  SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, 127, 127, 127));
  assert(SHPHandleGetFrame(&ctx->renderer->itemShapes, &frame, frameId));
  assert(SHPFrameGetImageData(&frame));

  SDL_RenderClear(r);
  SDL_SetRenderDrawColor(r, 127, 127, 127, 255);
  SDL_RenderFillRect(r, NULL);
  drawSHPFrameCursor(r, &frame, 0, 0, ctx->renderer->defaultPalette);
  ctx->renderer->cursor = SDL_CreateColorCursor(s, frameId == 0 ? 0 : w / 2,
                                                frameId == 0 ? 0 : w / 2);
  SDL_SetCursor(ctx->renderer->cursor);
  SDL_DestroyRenderer(r);
  SDL_FreeSurface(s);
  SHPFrameRelease(&frame);
  if (prevCursor) {
    SDL_FreeCursor(prevCursor);
  }
}

void renderCPSPart(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
                   const uint8_t *paletteBuffer, int destX, int destY,
                   int sourceX, int sourceY, int imageW, int imageH,
                   int sourceImageWidth) {

  void *data;
  int pitch;
  SDL_LockTexture(pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < imageW; x++) {
    for (int y = 0; y < imageH; y++) {
      int offset = (sourceImageWidth * (y + sourceY)) + x + sourceX;
      if (offset >= dataSize) {
        // printf("Offset %i >= %zu\n", offset, dataSize);
        continue;
      }
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

      drawPix(data, pitch, r, g, b, destX + x, destY + y);
    }
  }
  SDL_UnlockTexture(pixBuf);
}

void renderCPSAt(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
                 const uint8_t *paletteBuffer, int destX, int destY,
                 int sourceW, int sourceH, int imageW, int imageH) {
  void *data;
  int pitch;
  SDL_Rect rect = {.x = destX, .y = destY, .w = imageW, .h = imageH};
  SDL_LockTexture(pixBuf, &rect, &data, &pitch);
  for (int x = 0; x < sourceW; x++) {
    for (int y = 0; y < sourceH; y++) {
      int offset = ((imageW)*y) + x;
      if (offset >= dataSize) {
        // printf("Offset %i >= %zu\n", offset, dataSize);
        continue;
      }
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
  SDL_LockTexture(gameCtx->renderer->pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < MAZE_COORDS_W; x++) {
    for (int y = 0; y < MAZE_COORDS_H; y++) {
      drawPix(data, pitch, 0, 0, 0, MAZE_COORDS_X + x, MAZE_COORDS_Y + y);
    }
  }
  SDL_UnlockTexture(gameCtx->renderer->pixBuf);
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

/*
 9 7 3 7 9
 8 6 2 6 8
 8 5 1 5 8
   4 0 4
     ^=party pos.
*/

typedef enum {
  DecorationIndex_M_SOUTH = 1,
  DecorationIndex_N_SOUTH = DecorationIndex_M_SOUTH,
  DecorationIndex_O_SOUTH = DecorationIndex_M_SOUTH,

  DecorationIndex_I_SOUTH = 2,
  DecorationIndex_J_SOUTH = DecorationIndex_I_SOUTH,
  DecorationIndex_K_SOUTH = DecorationIndex_I_SOUTH,

  DecorationIndex_D_SOUTH = 3,
  DecorationIndex_C_SOUTH = DecorationIndex_D_SOUTH,
  DecorationIndex_E_SOUTH = DecorationIndex_D_SOUTH,

  DecorationIndex_P_EAST = 4,
  DecorationIndex_Q_WEST = DecorationIndex_P_EAST,

  DecorationIndex_M_WEST = 5,
  DecorationIndex_O_EAST = DecorationIndex_M_WEST,

  DecorationIndex_K_EAST = 6,

} DecorationIndex;

typedef struct {
  CELL_ID cellId;
  Orientation orientation;
  WallRenderIndex wallRenderIndex;
  DecorationIndex decoIndex;
  int x;
  int y;
  int xFlip;
} RenderWall;

static void renderDecoration(SDL_Texture *pixBuf, LevelContext *level,
                             const RenderWall *wall, uint16_t decorationId) {
  const DatDecoration *deco = level->datHandle.datDecoration + decorationId;
  if (deco->shapeIndex[wall->decoIndex] != DECORATION_EMPTY_INDEX) {
    SHPFrame frame = {0};
    size_t index = deco->shapeIndex[wall->decoIndex];
    SHPHandleGetFrame(&level->shpHandle, &frame, index);
    SHPFrameGetImageData(&frame);
    drawSHPMazeFrame(pixBuf, &frame, deco->shapeX[wall->decoIndex] + wall->x,
                     deco->shapeY[wall->decoIndex] + wall->y,
                     level->vcnHandle.palette, wall->xFlip, 1);
    int isFrontWall = (wall->cellId == CELL_N || wall->cellId == CELL_J ||
                       wall->cellId == CELL_D);
    if (isFrontWall && deco->flags & DatDecorationFlags_Mirror) {
      drawSHPMazeFrame(pixBuf, &frame, deco->shapeX[wall->decoIndex] + wall->x,
                       deco->shapeY[wall->decoIndex] + wall->y,
                       level->vcnHandle.palette, 1, 1);
    }
    SHPFrameRelease(&frame);
  }
  if (deco->next) {
    renderDecoration(pixBuf, level, wall, deco->next);
  }
}

static void renderWallDecoration(SDL_Texture *pixBuf, LevelContext *level,
                                 const RenderWall *wall, uint8_t wmi) {
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&level->wllHandle, wmi);
  if (mapping && mapping->decorationId != 0 &&
      mapping->decorationId < level->datHandle.nbrDecorations) {
    renderDecoration(pixBuf, level, wall, mapping->decorationId);
  }
}

static void computeViewConeCells(GameContext *gameCtx, int x, int y) {
  memset(gameCtx->renderer->viewConeEntries, 0,
         sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
  for (int i = 0; i < VIEW_CONE_NUM_CELLS; i++) {
    Point p;
    BlockGetCoordinates(&p.x, &p.y, gameCtx->currentBock, 0x80, 0x80);
    GetRealCoords(p.x, p.y, &p.x, &p.y);
    p = PointGo(&p, gameCtx->orientation, viewConeCell[i].frontDist,
                viewConeCell[i].leftDist);
    if (p.x >= 0 && p.x < 32 && p.y >= 0 && p.y < 32) {
      gameCtx->renderer->viewConeEntries[i].coords = p;
      gameCtx->renderer->viewConeEntries[i].valid = 1;
    }
  }
}

static const uint8_t monsterDirFlags[] = {0x08, 0x14, 0x00, 0x04, 0x04, 0x08,
                                          0x14, 0x00, 0x00, 0x04, 0x08, 0x14,
                                          0x14, 0x00, 0x04, 0x08};

static void renderEnemy(GameContext *gameCtx, const Monster *monster,
                        int monsterIndex, int cellId) {
  const ViewConeCell *cell = &viewConeCell[cellId];
  int x = 62;
  int y = 8;
  float ratioX = 1.0f;
  float ratioY = 1.0f;
  switch (cell->frontDist) {
  case 1:
    break;
  case 2:
    ratioX = 1.6;
    ratioY = 1.6; // 50
    x = 70;
    y = 16;
    x -= cell->leftDist * 80;
    break;
  case 3:
    ratioX = 2.2;
    ratioY = 2.2;
    x = 74;
    y = 22;
    x -= cell->leftDist * 40;
    break;
  }
  const MonsterProperties *props =
      gameCtx->level->monsterProperties + monster->type;
  assert(props);
  const SHPHandle *shp = &gameCtx->level->monsterShapes[props->shapeIndex];
  assert(shp);
  SHPFrame f = {0};

  int16_t frameIdx =
      monsterDirFlags[(gameCtx->orientation << 2) + monster->orientation];
  SHPHandleGetFrame(shp, &f, frameIdx);
  SHPFrameGetImageData(&f);
  if (cell->frontDist > 1) {
    SHPFrameScale(&f, f.header.width / ratioX, f.header.height / ratioY);
  }
  float att = 1.0f;
  if (cell->frontDist > 1) {
    att = 1.5f * abs(cell->frontDist);
  }

  drawSHPMazeFrame(gameCtx->renderer->pixBuf, &f, x, y,
                   gameCtx->level->vcnHandle.palette, 0, att);
  SHPFrameRelease(&f);
}

static void renderEnemies(GameContext *gameCtx, int blockId, int cellId) {
  for (int i = 0; i < MAX_MONSTERS; i++) {
    const Monster *monster = gameCtx->level->monsters + i;
    if (monster->block == blockId) {
      renderEnemy(gameCtx, monster, i, cellId);
    }
  }
}

static void renderDoor(GameContext *gameCtx, int cellId) {
  const ViewConeCell *cell = &viewConeCell[cellId];
  int x = 52;
  int y = 16;
  float ratioX = 1.0f;
  float ratioY = 1.0f;
  switch (cell->frontDist) {
  case 1:
    break;
  case 2:
    ratioX = 1.38;
    ratioY = 1.45;
    x = 62;
    y = 24;

    x -= cell->leftDist * 80;
    break;
  case 3:
    ratioX = 2.11;
    ratioY = 2.41;
    x = 71;
    y = 30;
    x -= cell->leftDist * 40;
    break;
  }
  SHPFrame frame = {0};
  SHPHandleGetFrame(&gameCtx->level->doors, &frame, 0);
  SHPFrameGetImageData(&frame);
  if (cell->frontDist > 1) {
    SHPFrameScale(&frame, frame.header.width / ratioX,
                  frame.header.height / ratioY);
  }
  float att = 1.f * abs(cell->frontDist);
  drawSHPMazeFrame(gameCtx->renderer->pixBuf, &frame, x, y,
                   gameCtx->level->vcnHandle.palette, 0, att);
  SHPFrameRelease(&frame);
}

static RenderWall renderWalls[] = {
    {CELL_A, East, A_east},
    {CELL_B, East, B_east},
    {CELL_C, East, C_east},
    {CELL_E, East, E_west},
    {CELL_F, West, F_west, 5, 0, 0, -1},
    {CELL_G, West, G_west},
    {CELL_B, South, B_south},
    {CELL_C, South, C_south, DecorationIndex_C_SOUTH, 16, 26, 0},
    {CELL_D, South, D_south, DecorationIndex_D_SOUTH, 64, 26, 0},
    {CELL_E, South, E_south, DecorationIndex_E_SOUTH, 17, 26, -1},
    {CELL_F, South, F_south},
    {CELL_H, East, H_east},
    {CELL_I, West, I_east},
    {CELL_K, West, K_west, DecorationIndex_K_EAST},
    {CELL_L, West, L_west},
    {CELL_I, South, I_south, DecorationIndex_I_SOUTH, -30, 20, 0},
    {CELL_J, South, J_south, DecorationIndex_J_SOUTH, 48, 20, 0},
    {CELL_K, South, K_south, DecorationIndex_K_SOUTH, 128, 20, 0},
    {CELL_M, East, M_east, DecorationIndex_M_WEST, 24, 10, 0},
    {CELL_O, West, O_west, DecorationIndex_O_EAST, 24, 10, 1},
    {CELL_M, South, M_south, DecorationIndex_M_SOUTH, 153, 8, 1},
    {CELL_N, South, N_south, DecorationIndex_N_SOUTH, 24, 8,
     0}, // the one in front
    {CELL_O, South, O_south, DecorationIndex_O_SOUTH, 153, 8, 0},
    {CELL_P, East, P_east, DecorationIndex_P_EAST, 0, 0, 0},
    {CELL_Q, West, Q_west, DecorationIndex_Q_WEST, 0, 0, 1},

};

void GameRenderMaze(GameContext *gameCtx) {
  clearMazeZone(gameCtx);
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 32; y++) {
      computeViewConeCells(gameCtx, x, y);
    }
  }
  LevelContext *level = gameCtx->level;
  drawCeilingAndFloor(gameCtx->renderer->pixBuf, &level->vcnHandle,
                      &level->vmpHandle);

  SDL_Texture *texture = gameCtx->renderer->pixBuf;

  for (int i = 0; i < sizeof(renderWalls) / sizeof(RenderWall); i++) {
    const RenderWall *r = renderWalls + i;
    const ViewConeEntry *entry = gameCtx->renderer->viewConeEntries + r->cellId;
    if (!entry) {
      continue;
    }
    int blockId = entry->coords.y * 32 + entry->coords.x;
    Orientation absOri = absOrientation(gameCtx->orientation, r->orientation);

    uint8_t wmi = gameCtx->level->blockProperties[blockId].walls[absOri];

    if (wmi) {
      uint16_t wallType = WllHandleGetWallType(&level->wllHandle, wmi);
      if (wallType) {

        if (r->orientation == South &&
            (r->cellId == CELL_N || r->cellId == CELL_J ||
             r->cellId == CELL_I || r->cellId == CELL_K ||
             r->cellId == CELL_C || r->cellId == CELL_E ||
             r->cellId == CELL_D) &&
            wallType == 3) { // door
          renderDoor(gameCtx, r->cellId);
        }
        drawWall(texture, &level->vcnHandle, &level->vmpHandle, wallType,
                 r->wallRenderIndex);
      }
      if (r->decoIndex != 0) {
        renderWallDecoration(texture, level, r, wmi);
      }
    }
    if (r->cellId == CELL_N || r->cellId == CELL_J || r->cellId == CELL_I ||
        r->cellId == CELL_K || r->cellId == CELL_C || r->cellId == CELL_E ||
        r->cellId == CELL_D) {
      renderEnemies(gameCtx, blockId, r->cellId);
    }
  }
}
