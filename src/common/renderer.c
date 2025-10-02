#include "renderer.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "game.h"
#include "geometry.h"
#include <SDL_image.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void renderPalette(SDL_Renderer *renderer, const uint8_t *paletteBuffer,
                          int offsetX, int offsetY) {
  for (int i = 0; i < 256; i++) {
    int x = i % 16;
    int y = i / 16;

    uint8_t r = VGA6To8(paletteBuffer[(i * 3) + 0]);
    uint8_t g = VGA6To8(paletteBuffer[(i * 3) + 1]);
    uint8_t b = VGA6To8(paletteBuffer[(i * 3) + 2]);
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {
        .x = offsetX + x * 10, .y = offsetY + y * 10, .w = 10, .h = 10};
    SDL_RenderFillRect(renderer, &rect);
  }
}

static void renderCPSImage(SDL_Renderer *renderer, const uint8_t *imgData,
                           size_t dataSize, const uint8_t *paletteBuffer, int w,
                           int h) {
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
      if (1) { // r && g && b) {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect rect = {.x = x * 2, .y = y * 2, .w = 2, .h = 2};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }
}

void WSAFrameToPng(const uint8_t *frame, size_t size, const uint8_t *palette,
                   const char *savePngPath, int w, int h) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface = SDL_CreateRGBSurface(0, 800, 400, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 255, 0, 0);
  SDL_RenderClear(renderer);

  renderCPSImage(renderer, frame, size, palette, w, h);
  if (palette) {
    renderPalette(renderer, palette, 640, 0);
  }
  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}

void CPSImageToPng(const CPSImage *image, const char *savePngPath) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface = SDL_CreateRGBSurface(0, 800, 400, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  renderCPSImage(renderer, image->data, image->imageSize, image->palette, 320,
                 200);
  if (image->palette) {
    renderPalette(renderer, image->palette, 640, 0);
  }
  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}

static void drawPix(SDL_Renderer *renderer, int x, int y, uint8_t r, uint8_t g,
                    uint8_t b) {
  SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  SDL_Rect rect;
  rect.w = 1;
  rect.h = 1;
  rect.x = MAZE_COORDS_X + x;
  rect.y = MAZE_COORDS_Y + y;
  if (r && g && b) {
    SDL_RenderFillRect(renderer, &rect);
  }
}
void blitBlock(SDL_Renderer *renderer, const VCNHandle *handle, int blockId,
               int x, int y, int flip) {
  const VCNBlock *block = handle->blocks + blockId;

  const int s = flip ? -1 : 1;
  const int p = flip ? 7 : 0;

  uint8_t numPalette = handle->blocksPalettePosTable[blockId] / 16;
  if (numPalette >= VCN_PALETTE_TABLE_SIZE) {
    printf("numPalette %i > VCN_PALETTE_TABLE_SIZE %i\n", numPalette,
           VCN_PALETTE_TABLE_SIZE);
  }
  assert(numPalette < VCN_PALETTE_TABLE_SIZE);

  for (int w = 0; w < 8; w++) {
    for (int v = 0; v < 4; v++) {
      uint8_t word = block->rawData[v + w * 4];
      uint8_t p0 = (word & 0xf0) >> 4;
      uint8_t p1 = word & 0x0f;

      uint8_t idx0 =
          handle->posPaletteTables[numPalette].backdropWallPalettes[p0];
      assert(idx0 < 128);
      uint8_t idx1 =
          handle->posPaletteTables[numPalette].backdropWallPalettes[p1];
      assert(idx1 < 128);

      uint8_t r = VGA6To8(handle->palette[0 + idx0 * 3]);
      uint8_t g = VGA6To8(handle->palette[1 + idx0 * 3]);
      uint8_t b = VGA6To8(handle->palette[2 + idx0 * 3]);

      uint8_t destX = x + p + s * 2 * v;
      uint8_t destY = y + w;

      drawPix(renderer, destX, destY, r, g, b);

      r = VGA6To8(handle->palette[0 + idx1 * 3]);
      g = VGA6To8(handle->palette[1 + idx1 * 3]);
      b = VGA6To8(handle->palette[2 + idx1 * 3]);

      destX = x + p + s + s * 2 * v;
      drawPix(renderer, destX, destY, r, g, b);
    }
  }
}

void drawChar(SDL_Renderer *renderer, const FNTHandle *font, uint16_t c,
              int xOff, int yOff) {
  if (c >= font->numGlyphs) {
    assert(0);
    return;
  }

  if (!font->bitmapOffsets[c]) {
    return;
  }
  const uint8_t *src = font->buffer + font->bitmapOffsets[c];
  const uint8_t charWidth = font->widthTable[c];

  if (!charWidth)
    return;

  uint8_t charH1 = font->heightTable[c * 2 + 0];
  uint8_t charH2 = font->heightTable[c * 2 + 1];
  uint8_t charH0 = font->maxHeight - (charH1 + charH2);

  int x = xOff;
  int y = yOff;
  while (charH1--) {
    uint8_t col = 1; //_colorMap[0];
    for (int i = 0; i < charWidth; ++i) {
      if (col != 0) {
        SDL_SetRenderDrawColor(renderer, col, 0, 0, 255);
        SDL_Rect r = {x, y, 1, 1};
        SDL_RenderFillRect(renderer, &r);
      }
      x++;
    }
    x = xOff;
    y += 1;
  }

  while (charH2--) {
    uint8_t b = 0;
    for (int i = 0; i < charWidth; ++i) {
      uint8_t col;
      if (i & 1) {
        col = b >> 4; //_colorMap[b >> 4];
      } else {
        b = *src++;
        col = b & 0XF; //_colorMap[b & 0xF];
      }
      if (col != 0) {
        SDL_SetRenderDrawColor(renderer, col, 0, 0, 255);
        SDL_Rect r = {x, y, 1, 1};
        SDL_RenderFillRect(renderer, &r);
      }
      x += 1;
    }
    y += 1;
    x = xOff;
  }

  while (charH0--) {
    uint8_t col = 100; //_colorMap[0];
    for (int i = 0; i < charWidth; ++i) {
      if (col != 0) {
        SDL_SetRenderDrawColor(renderer, col, 0, 0, 255);
        SDL_Rect r = {x, y, 1, 1};
        SDL_RenderFillRect(renderer, &r);
      }
      x += 1;
    }
    y += 1;
    x = xOff;
  }
}

void FNTToPng(const FNTHandle *font, const char *savePngPath, int charNum) {
  const int imgWidth = 400;
  const int imgHeight = 300;
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface =
      SDL_CreateRGBSurface(0, imgWidth, imgHeight, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  int x = 10;
  int y = 10;
  for (int i = 0; i < font->numGlyphs; i++) {
    if (i % 32 == 0) {
      y += 10;
      x = 10;
    }
    drawChar(renderer, font, i, x, y);
    x += 10;
  }

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}

#define BLOCK_SIZE (int)8

void VCNImageToPng(const VCNHandle *handle, const char *savePngPath) {
  const int widthBlocks = 32;
  const int heightBlocks = (int)handle->nbBlocks / 32;
  const int imageWidth = widthBlocks * BLOCK_SIZE;
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface = SDL_CreateRGBSurface(
      0, imageWidth, heightBlocks * BLOCK_SIZE, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  for (int i = 0; i < handle->nbBlocks; i++) {
    int blockX = i % widthBlocks;
    int blockY = i / widthBlocks;
    blitBlock(renderer, handle, i, blockX * BLOCK_SIZE, blockY * BLOCK_SIZE, 0);
  }

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}

void drawSHPFrame(SDL_Renderer *renderer, const SHPFrame *frame, int xPos,
                  int yPos, const uint8_t *palette, int scaleFactor,
                  uint8_t xFlip) {

  for (int y = 0; y < frame->header.height; y++) {
    for (int x = 0; x < frame->header.width; x++) {
      int p = x + (y * frame->header.width);
      uint8_t v = frame->imageBuffer[p];
      if (v == 0) {
        continue;
      }
      uint8_t r = v;
      uint8_t g = v;
      uint8_t b = v;
      if (palette) {
        r = VGA6To8(palette[(v * 3) + 0]);
        g = VGA6To8(palette[(v * 3) + 1]);
        b = VGA6To8(palette[(v * 3) + 2]);
      }
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect;
      rect.w = scaleFactor;
      rect.h = scaleFactor;
      rect.x = (x + xPos) * scaleFactor;
      if (xFlip) {
        rect.x = MAZE_COORDS_W - rect.x;
      }
      rect.x += MAZE_COORDS_X;
      rect.y = (y + yPos) * scaleFactor;
      rect.y += MAZE_COORDS_Y;
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}
void SHPFrameToPng(const SHPFrame *frame, const char *savePngPath,
                   const uint8_t *palette) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface = SDL_CreateRGBSurface(
      0, frame->header.width, frame->header.height, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  drawSHPFrame(renderer, frame, 0, 0, palette, 1, 0);

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
  int baseOffset;
  int offsetInViewPort;
  int visibleHeightInBlocks;
  int visibleWidthInBlocks;
  int skipValue;
  int flipFlag;
} WallRenderData;
WallRenderData wallRenderData[25] = /* 25 different wall positions exists */
    {
        /* Side-Walls left back */
        {3, 66, 5, 1, 2, 0},  /* A-east */
        {1, 68, 5, 3, 0, 0},  /* B-east */
        {-4, 74, 5, 1, 0, 0}, /* C-east */

        /* Side-Walls right back */
        {-4, 79, 5, 1, 0, 1}, /* E-west */
        {1, 83, 5, 3, 0, 1},  /* F-west */
        {3, 87, 5, 1, 2, 1},  /* G-west */

        /* Frontwalls back */
        {32, 66, 5, 2, 4, 0}, /* B-south */
        {28, 68, 5, 6, 0, 0}, /* C-south */
        {28, 74, 5, 6, 0, 0}, /* D-south */
        {28, 80, 5, 6, 0, 0}, /* E-south */
        {28, 86, 5, 2, 4, 0}, /* F-south */

        /* Side walls middle back left */
        {16, 66, 6, 2, 0, 0},  /* H-east */
        {-20, 50, 8, 2, 0, 0}, /* I-east */

        /* Side walls middle back right */
        {-20, 58, 8, 2, 0, 1}, /* K-west */
        {16, 86, 6, 2, 0, 1},  /* L-west */

        /* Frontwalls middle back */
        {62, 44, 8, 6, 4, 0},  /* I-south */
        {58, 50, 8, 10, 0, 0}, /* J-south */
        {58, 60, 8, 6, 4, 0},  /* K-south */

        /* Side walls middle front left */
        {-56, 25, 12, 3, 0, 0}, /* M-east */

        /* Side walls middle front right */
        {-56, 38, 12, 3, 0, 1}, /* O-west */

        /* Frontwalls middle front */
        {151, 22, 12, 3, 13, 0}, /* M-south */
        {138, 41, 12, 3, 13, 0}, /* O-south */
        {138, 25, 12, 16, 0, 0}, /* N-south */

        /* Side wall front left */
        {-101, 0, 15, 3, 0, 0}, /* P-east */

        /* Side wall front right */
        {-101, 19, 15, 3, 0, 1}, /* Q-west */
};

void drawWall(GameContext *gameCtx, const VCNHandle *vcn, const VMPHandle *vmp,
              int wallType, int wallPosition) {
  SDL_Renderer *renderer = gameCtx->renderer;
  const WallRenderData *wallCfg = &wallRenderData[wallPosition];
  int flipX = wallCfg->flipFlag;
  int offset = wallCfg->baseOffset;

  for (int y = 0; y < wallCfg->visibleHeightInBlocks; y++) {
    for (int x = 0; x < wallCfg->visibleWidthInBlocks; x++) {
      int blockIndex;
      if (flipX == 0) {
        blockIndex = x + y * 22 + wallCfg->offsetInViewPort;
      } else {
        blockIndex = wallCfg->offsetInViewPort + wallCfg->visibleWidthInBlocks -
                     1 - x + y * 22;
      }

      int xpos = blockIndex % 22;
      int ypos = blockIndex / 22;
      VMPTile tile = {0};
      VMPHandleGetTile(vmp, offset + (431 * wallType), &tile);
      // int tile = vmp-> .wallTiles[wallType][offset];

      /* xor with wall flip-x to make block flip and wall flip cancel each other
       * out. */
      int blockFlip = (tile.flipped) ^ flipX;
      int destPostX = xpos * 8;
      int destPostY = ypos * 8;
      blitBlock(renderer, vcn, tile.blockIndex, destPostX, destPostY,
                blockFlip);

      offset++;
    }
    offset += wallCfg->skipValue;
  }
}

void drawBackground(SDL_Renderer *renderer, const VCNHandle *vcn,
                    const VMPHandle *vmp) {
  for (int y = 0; y < 15; y++) {
    for (int x = 0; x < 22; x++) {
      int index = y * 22 + x;

      VMPTile tile = {0};
      VMPHandleGetTile(vmp, index, &tile);
      assert(tile.blockIndex < vcn->nbBlocks);
      blitBlock(renderer, vcn, tile.blockIndex, x * 8, y * 8, tile.flipped);
    }
  }
}
