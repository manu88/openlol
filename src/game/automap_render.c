#include "automap_render.h"
#include "SDL_pixels.h"
#include "display.h"
#include "formats/format_shp.h"
#include "formats/format_xxx.h"
#include "game_ctx.h"
#include "geometry.h"
#include "level.h"
#include "menu.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"
#include <stdint.h>
#include <stdio.h>

#define BLOCK_W 7
#define BLOCK_H 6

static int _automapTopLeftX = 0;
static int _automapTopLeftY = 0;

static int mapGetStartPosX(const GameContext *gameCtx) {
  int c = 0;
  int a = 32;

  do {
    for (a = 0; a < 32; a++) {
      if (gameCtx->level->blockProperties[(a << 5) + c].flags)
        break;
    }
    if (a == 32)
      c++;
  } while (c < 32 && a == 32);

  int d = 31;

  do {
    for (a = 0; a < 32; a++) {
      if (gameCtx->level->blockProperties[(a << 5) + d].flags)
        break;
    }
    if (a == 32)
      d--;
  } while (d > 0 && a == 32);

  _automapTopLeftX = (d > c) ? ((32 - (d - c)) >> 1) * 7 + 5 : 5;
  return (d > c) ? c : 0;
}

static int mapGetStartPosY(const GameContext *gameCtx) {
  int c = 0;
  int a = 32;

  do {
    for (a = 0; a < 32; a++) {
      if (gameCtx->level->blockProperties[(c << 5) + a].flags)
        break;
    }
    if (a == 32)
      c++;
  } while (c < 32 && a == 32);

  int d = 31;

  do {
    for (a = 0; a < 32; a++) {
      if (gameCtx->level->blockProperties[(d << 5) + a].flags)
        break;
    }
    if (a == 32)
      d--;
  } while (d > 0 && a == 32);

  _automapTopLeftY = (d > c) ? ((32 - (d - c)) >> 1) * 6 + 4 : 4;
  return (d > c) ? c : 0;
}

static const int8_t mapCoords[12][4] = {
    {0, 7, 0, -5}, {-5, 0, 6, 0}, {7, 5, 7, 1},    {5, 6, 4, 6},
    {0, 7, 0, -1}, {-3, 0, 6, 0}, {6, 7, 6, -3},   {-3, 5, 6, 5},
    {1, 5, 1, 1},  {3, 1, 3, 1},  {-1, 6, -1, -8}, {-7, -1, 5, -1}};

#define NUM_LEGENDS 11
static uint16_t legendTypes[NUM_LEGENDS];

static void addLegendData(uint16_t type) {
  type &= 0x7F;
  for (int i = 0; i < NUM_LEGENDS; i++) {
    if (legendTypes[i] == type) {
      return;
    }
    if (legendTypes[i] == 0) {
      legendTypes[i] = type;
      return;
    }
  }
}

// returns - 1 if invalid
static int automapToIndex(uint16_t automapData) {
  automapData &= 0x1F;
  if (automapData == 0x1F) {
    return -1;
  }
  return automapData * 4; // we multiply by 4 because they are 4 different
                          // orientations (N/S/E/W) per automap data.
}

static void drawMapShape(const Display *ctx, int index, int x, int y,
                         int direction) {
  if (index < 0) {
    return;
  }
  SHPFrame f = {0};
  x = x + mapCoords[10][direction] - 2;
  y = y + mapCoords[11][direction] - 2;
  SHPHandleGetFrame(&ctx->automapShapes, &f, index + 11 + direction);
  SHPFrameGetImageData(&f);
  drawSHPFrame(ctx->pixBuf, &f, x, y, ctx->defaultPalette);
  SHPFrameRelease(&f);
}

void mapOverlay(SDL_Texture *texture, int startX, int startY, int w, int h) {
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      row[x] -= 0X00081814;
      // 158	115	69
    }
  }
  SDL_UnlockTexture(texture);
}

void colorBlock(SDL_Texture *texture, int startX, int startY, int w, int h,
                SDL_Color col) {
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      row[x] = 0XFF000000 + (col.r << 0X10) + (col.g << 0X8) + col.b;
      // 158	115	69
    }
  }
  SDL_UnlockTexture(texture);
}

void AutomapRender(GameContext *gameCtx) {
  memset(legendTypes, 0, NUM_LEGENDS * sizeof(uint16_t));
  UISetDefaultStyle();
  renderCPS(gameCtx->display->pixBuf, gameCtx->display->mapBackground.data,
            gameCtx->display->mapBackground.imageSize,
            gameCtx->display->mapBackground.palette, PIX_BUF_WIDTH,
            PIX_BUF_HEIGHT);

  char c[20] = "";
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
               MAP_SCREEN_EXIT_BUTTON_X + 2, MAP_SCREEN_BUTTONS_Y + 4, 50, c);

  GameContextGetLevelName(gameCtx, c, sizeof(c));
  UIRenderText(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
               MAP_SCREEN_NAME_X, MAP_SCREEN_NAME_Y, 320 - MAP_SCREEN_NAME_Y,
               c);

  if (gameCtx->conf.debug) {

    snprintf(c, 20, "0X%X 0X%X", gameCtx->currentBock, gameCtx->orientation);
    UIRenderText(&gameCtx->display->defaultFont, gameCtx->display->pixBuf, 10,
                 10, 320, c);
  }
  uint16_t blX = mapGetStartPosX(gameCtx);
  uint16_t bl = (mapGetStartPosY(gameCtx) << 5) + blX;

  int sx = _automapTopLeftX;
  int sy = _automapTopLeftY;

  for (; bl < MAZE_NUM_CELL; bl++) {
    const uint8_t *w = gameCtx->level->blockProperties[bl].walls;

    const WllWallMapping *w0Mapping =
        WllHandleGetWallMapping(&gameCtx->level->wllHandle, w[0]);
    const WllWallMapping *w1Mapping =
        WllHandleGetWallMapping(&gameCtx->level->wllHandle, w[1]);
    const WllWallMapping *w2Mapping =
        WllHandleGetWallMapping(&gameCtx->level->wllHandle, w[2]);
    const WllWallMapping *w3Mapping =
        WllHandleGetWallMapping(&gameCtx->level->wllHandle, w[3]);
    uint16_t automapW0 = w0Mapping ? w0Mapping->automapData : 0;
    uint16_t automapW1 = w1Mapping ? w1Mapping->automapData : 0;
    uint16_t automapW2 = w2Mapping ? w2Mapping->automapData : 0;
    uint16_t automapW3 = w3Mapping ? w3Mapping->automapData : 0;

    if ((!(automapW0 & 0xC0)) && (!(automapW1 & 0xC0)) &&
        (!(automapW2 & 0xC0)) && (!(automapW3 & 0xC0))) {
      uint16_t northBlock = BlockCalcNewPosition(bl, North);
      uint16_t southBlock = BlockCalcNewPosition(bl, South);
      uint16_t eastBlock = BlockCalcNewPosition(bl, East);
      uint16_t westBlock = BlockCalcNewPosition(bl, West);

      uint8_t northWall =
          gameCtx->level->blockProperties[northBlock].walls[South];
      uint8_t southWall =
          gameCtx->level->blockProperties[southBlock].walls[North];
      uint8_t eastWall = gameCtx->level->blockProperties[eastBlock].walls[West];
      uint8_t westWall = gameCtx->level->blockProperties[westBlock].walls[East];

      const WllWallMapping *eastMapping =
          WllHandleGetWallMapping(&gameCtx->level->wllHandle, eastWall);
      const WllWallMapping *southMapping =
          WllHandleGetWallMapping(&gameCtx->level->wllHandle, southWall);
      const WllWallMapping *westMapping =
          WllHandleGetWallMapping(&gameCtx->level->wllHandle, westWall);
      const WllWallMapping *northMapping =
          WllHandleGetWallMapping(&gameCtx->level->wllHandle, northWall);

      if (eastMapping) {
        if (eastMapping->automapData & 0xC0) {
          UIFillRect(gameCtx->display->pixBuf, sx + BLOCK_W - 1, sy, 1, BLOCK_H,
                     (SDL_Color){.0, 0, 0});
        }
        drawMapShape(gameCtx->display, automapToIndex(eastMapping->automapData),
                     sx, sy, East);
      }
      if (northMapping) {
        if (northMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->display->pixBuf, sx, sy, BLOCK_W, 1,
                     (SDL_Color){.0, 0, 0});
        }
        drawMapShape(gameCtx->display,
                     automapToIndex(northMapping->automapData), sx, sy, North);
      }

      if (southMapping) {
        if (southMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->display->pixBuf, sx, sy + BLOCK_H - 1, BLOCK_W, 1,
                     (SDL_Color){.0, 0, 0});
        }
        drawMapShape(gameCtx->display,
                     automapToIndex(southMapping->automapData), sx, sy, South);
      }
      if (westMapping) {
        if (westMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->display->pixBuf, sx, sy, 1, BLOCK_H,
                     (SDL_Color){.0, 0, 0});
        }
        drawMapShape(gameCtx->display, automapToIndex(westMapping->automapData),
                     sx, sy, West);
      }
      mapOverlay(gameCtx->display->pixBuf, sx, sy, BLOCK_W, BLOCK_H);
      if (gameCtx->conf.showMonstersInMap) {
        for (int i = 0; i < MAX_MONSTERS; i++) {
          const Monster *m = &gameCtx->level->monsters[i];
          if (m->block == bl) {
            colorBlock(gameCtx->display->pixBuf, sx, sy, BLOCK_W, BLOCK_H,
                       (SDL_Color){.r = 255, 0, 0});
          }
        }
      }
    }

    sx += BLOCK_W;
    if (bl % 32 == 31) {
      sx = _automapTopLeftX;
      sy += BLOCK_H;
      bl += blX;
    }
  } // end of for (; bl < 1024; bl++)

  sx = mapGetStartPosX(gameCtx);
  sy = mapGetStartPosY(gameCtx);

  int partyX =
      _automapTopLeftX + (((gameCtx->currentBock - sx) % 32) * BLOCK_W);
  int partY =
      _automapTopLeftY + (((gameCtx->currentBock - (sy << 5)) / 32) * BLOCK_H);

  drawMapShape(gameCtx->display, 48 + gameCtx->orientation, partyX,
               partY + BLOCK_H, 0);

  printf("Got %zu legend entries\n", gameCtx->level->legendData.numEntries);
  char buf[64];
  for (int i = 0; i < NUM_LEGENDS; i++) {
    if (legendTypes[i] == 0) {
      continue;
    }
    printf("legend %i: 0X%X\n", i, legendTypes[i]);
    for (int j = 0; j < gameCtx->level->legendData.numEntries; j++) {

      LegendEntry *entry = gameCtx->level->legendData.entries + j;
      if (entry->shapeId == legendTypes[i]) {
        printf("string id = %i\n", entry->stringId - 0X4000);
        GameContextGetString(gameCtx, entry->stringId, buf, 64);
        printf("'%s'\n", buf);
        break;
      }
    }
  }
}
