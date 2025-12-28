#include "automap_render.h"
#include "SDL_pixels.h"
#include "formats/format_shp.h"
#include "geometry.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"

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
  a = 32;

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
  a = 32;

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

static void drawMapShape(const GameContext *gameCtx, uint16_t automapData,
                         int x, int y, int direction) {
  automapData &= 0x1F;
  if (automapData == 0x1F)
    return;
  SHPFrame f = {0};
  SHPHandleGetFrame(&gameCtx->automapShapes, &f,
                    ((automapData << 2) + direction) + 11);
  SHPFrameGetImageData(&f);
  x = x + mapCoords[10][direction] - 2;
  y = y + mapCoords[11][direction] - 2;
  drawSHPFrame(gameCtx->pixBuf, &f, x, y, gameCtx->defaultPalette);
  SHPFrameRelease(&f);
  //_screen->drawShape(_screen->_curPage, _automapShapes[(l << 2) + direction],
  // x + _mapCoords[10][direction] - 2, y + _mapCoords[11][direction] - 2, 0,
  // 0);
}

#define BLOCK_W 7
#define BLOCK_H 6

void AutomapRender(GameContext *gameCtx) {
  UISetDefaultStyle();
  renderCPS(gameCtx->pixBuf, gameCtx->mapBackground.data,
            gameCtx->mapBackground.imageSize, gameCtx->mapBackground.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  char c[20] = "";
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf,
               MAP_SCREEN_EXIT_BUTTON_X + 2, MAP_SCREEN_BUTTONS_Y + 4, 50, c);

  GameContextGetLevelName(gameCtx, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, MAP_SCREEN_NAME_X,
               MAP_SCREEN_NAME_Y, 320 - MAP_SCREEN_NAME_Y, c);

  uint16_t blX = mapGetStartPosX(gameCtx);
  uint16_t bl = (mapGetStartPosY(gameCtx) << 5) + blX;

  int sx = _automapTopLeftX;
  int sy = _automapTopLeftY;

  for (; bl < 1024; bl++) {
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

      SDL_Color c = {.r = 255, .g = 255, .b = 255};
      if (bl == gameCtx->currentBock) {
        c = (SDL_Color){.r = 0, .g = 255, .b = 0};
      }
      UIFillRect(gameCtx->pixBuf, sx, sy, BLOCK_W, BLOCK_H, c);
      if (eastMapping) {
        drawMapShape(gameCtx, eastMapping->automapData, sx, sy, East);
        if (eastMapping->automapData & 0xC0) {
          UIFillRect(gameCtx->pixBuf, sx + BLOCK_W - 1, sy, 1, BLOCK_H,
                     (SDL_Color){.r = 255});
        }
      }
      if (northMapping) {
        drawMapShape(gameCtx, northMapping->automapData, sx, sy, North);
        if (northMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->pixBuf, sx, sy, BLOCK_W, 1,
                     (SDL_Color){.r = 255});
        }
      }

      if (southMapping) {
        drawMapShape(gameCtx, southMapping->automapData, sx, sy, South);
        if (southMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->pixBuf, sx, sy + BLOCK_H - 1, BLOCK_W, 1,
                     (SDL_Color){.r = 255});
        }
      }
      if (westMapping) {
        drawMapShape(gameCtx, westMapping->automapData, sx, sy, West);
        if (westMapping->automapData & 0XC0) {
          UIFillRect(gameCtx->pixBuf, sx, sy, 1, BLOCK_H,
                     (SDL_Color){.r = 255});
        }
      }
    }
    sx += 7;
    if (bl % 32 == 31) {
      sx = _automapTopLeftX;
      sy += 6;
      bl += blX;
    }
  } // end of for (; bl < 1024; bl++)

  int partyX = _automapTopLeftX + (((gameCtx->currentBock - sx) % 32) * 7);
  int partY =
      _automapTopLeftY + (((gameCtx->currentBock - (sy << 5)) / 32) * 6);

  drawMapShape(gameCtx, 48 + gameCtx->orientation, partyX - 3, partY - 2, 0);
}
