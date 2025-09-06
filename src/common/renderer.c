#include "renderer.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <assert.h>

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

static void renderVCN(SDL_Renderer *renderer, const VCNHandle *data,
                      int blockId, int xOff, int yOff) {
  const Block *blk = data->blocks + blockId;
  uint8_t numPalette =
      (data->blocksPalettePosTable + blockId)->numPalettes[0] / 16;
  assert(numPalette < VCN_PALETTE_TABLE_SIZE);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 4; x++) {
      uint8_t rawData = blk->rawData[x][y];
      uint8_t p0 = rawData >> 4;
      uint8_t p1 = rawData & 0X0F;

      uint8_t idx0 =
          data->posPaletteTables[numPalette].backdropWallPalettes[p0];
      assert(idx0 < 128);

      uint8_t idx1 =
          data->posPaletteTables[numPalette].backdropWallPalettes[p1];
      assert(idx1 < 128);

      {
        uint8_t r = VGA6To8(data->palette[0 + idx0 * 3]);
        uint8_t g = VGA6To8(data->palette[1 + idx0 * 3]);
        uint8_t b = VGA6To8(data->palette[2 + idx0 * 3]);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect rect = {
            .x = xOff + ((x * 2) * 2), .y = yOff + (y * 2), .w = 2, .h = 2};
        SDL_RenderFillRect(renderer, &rect);
      }
      {
        uint8_t r = VGA6To8(data->palette[0 + idx1 * 3]);
        uint8_t g = VGA6To8(data->palette[1 + idx1 * 3]);
        uint8_t b = VGA6To8(data->palette[2 + idx1 * 3]);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect rect = {
            .x = xOff + ((1 + x * 2) * 2), .y = yOff + (y * 2), .w = 2, .h = 2};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }
}
void VCNImageToPng(const VCNHandle *handle, const char *savePngPath) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *surface =
      SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGH, 32, 0, 0, 0, 0);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);

  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  for (int i = 0; i < handle->nbBlocks; i++) {

    int xOff = (i % 32) * 20;
    int yOff = (i / 32) * 20;
    renderVCN(renderer, handle, i, xOff, yOff);
  }

  SDL_RenderPresent(renderer);
  IMG_SavePNG(surface, savePngPath);

  SDL_DestroyRenderer(renderer);
}
