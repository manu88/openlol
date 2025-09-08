#include "renderer.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include "format_vcn.h"
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
               int imgWidth, int x, int y, int flip) {
  const VCNBlock *block = handle->blocks + blockId;

  const int s = flip ? -1 : 1;
  const int p = flip ? 7 : 0;

  const uint8_t numPalette = handle->blocksPalettePosTable[blockId] / 16;
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

    blitBlock(renderer, handle, i, imageWidth, blockX * BLOCK_SIZE,
              blockY * BLOCK_SIZE, 0);
  }

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}
