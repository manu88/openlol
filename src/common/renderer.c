#include "renderer.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <assert.h>
#include <stdint.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGH 400

static inline uint8_t VGA6To8(uint8_t v) { return (v * 255) / 63; }
#define VGA8To8(x) x

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
                           const uint8_t *paletteBuffer) {
  for (int x = 0; x < 320; x++) {
    for (int y = 0; y < 200; y++) {
      uint8_t paletteIdx = *(imgData + (320 * y) + x);
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

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect = {.x = x * 2, .y = y * 2, .w = 2, .h = 2};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

void CPSImageToPng(const CPSImage *image, const char *savePngPath) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface =
      SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGH, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  renderCPSImage(renderer, image->data, image->palette);
  if (image->palette) {
    renderPalette(renderer, image->palette, 640, 0);
  }
  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
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
      // int coords = x + p + s * 2 * v + (y + w) * img_width;

      uint8_t r = VGA6To8(handle->palette[0 + idx0 * 3]);
      uint8_t g = VGA6To8(handle->palette[1 + idx0 * 3]);
      uint8_t b = VGA6To8(handle->palette[2 + idx0 * 3]);

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect;
      rect.w = 1;
      rect.h = 1;
      rect.x = x + p + s * 2 * v;
      rect.y = y + w;
      SDL_RenderFillRect(renderer, &rect);

      r = VGA6To8(handle->palette[0 + idx1 * 3]);
      g = VGA6To8(handle->palette[1 + idx1 * 3]);
      b = VGA6To8(handle->palette[2 + idx1 * 3]);

      rect.x = x + p + s + s * 2 * v;
      rect.y = y + w;
      SDL_RenderFillRect(renderer, &rect);
    }
  }
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct WallRenderData {
  int baseOffset;
  int offsetInViewPort;
  int visibleWidthInBlocks;
  int visibleHeightInBlocks;
  int skipValue;
  int flipFlag;
} wallRenderData[25] = /* 25 different wall positions exists */
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

static void drawBackground(const VCNHandle *vcn, const VMPHandle *vmp) {
  const int width = 22 * 8;
  const int height = 15 * 8;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  for (int y = 0; y < 15; y++) {
    for (int x = 0; x < 22; x++) {
      int index = y * 22 + x;

      VMPTile tile = {0};
      VMPHandleGetTile(vmp, index, &tile);

      assert(tile.blockIndex < vcn->nbBlocks);

      blitBlock(renderer, vcn, tile.blockIndex, x * 8, y * 8, tile.flipped);
    }
  }

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, "render.png");

  SDL_DestroyRenderer(renderer);
}

void testRenderScene(const VCNHandle *vcn, const VMPHandle *vmp) {
  drawBackground(vcn, vmp);
}
