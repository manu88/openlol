#include "format_cps.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "bytes.h"
#include "format_lcw.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void writeImage(const uint8_t *imgData, size_t imgDataSize,
                       uint8_t *paletteBuffer);

static void swapBuffer(uint8_t *buffer, size_t bufferSize) {
  // assert(bufferSize % 2 == 0);
  uint16_t *start = (uint16_t *)buffer;
  size_t size = bufferSize / 2;
  for (int i = 0; i < size; i++) {
    uint16_t c = swap_uint16(start[i]);
    uint8_t c0 = c >> 8;
    uint8_t c1 = c & 0XFF;
    buffer[i * 2] = c0;
    buffer[i * 2 + 1] = c1;
  }
}
uint8_t *TestCps(const uint8_t *buffer, size_t bufferSize) {
  CPSFileHeader *file = (CPSFileHeader *)buffer;
  assert(file->compressionType <= CPSCompressionType_LZW_LCW);
  file->fileSize += 2; // CPSCompressionType_LZW_LCW: must add 2 bytes
  printf("test CPS\n");
  printf("Compressed buffer size %zi\n", bufferSize);
  printf("file size %i\n", file->fileSize);
  printf("compressionType %i\n", file->compressionType);
  printf("uncompressedSize %i\n", file->uncompressedSize);
  printf("paletteSize %i\n", file->paletteSize);
  assert(file->uncompressedSize == 64000);
  assert(file->paletteSize == 0 ||
         file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA);

  size_t dataSize = bufferSize - 10;

  uint8_t *dataBuffer = (uint8_t *)buffer + 10;

  uint8_t *paletteBuffer = dataBuffer; // malloc(PALETTE_SIZE_256_6_RGB_VGA);
  swapBuffer((uint8_t *)paletteBuffer, PALETTE_SIZE_256_6_RGB_VGA);
  if (file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA) {
    dataBuffer += file->paletteSize;
    dataSize -= file->paletteSize;
  }

  uint8_t *dest = malloc(file->uncompressedSize);
  assert(dest);
  // LCWDecompress(const uint8_t *source, size_t sourceSize, uint8_t *dest,
  // size_t destSize)
  assert(dataBuffer[dataSize - 1] == 0X80);

  int bytes = LCWDecompress(dataBuffer, dataSize, dest, file->uncompressedSize);
  printf("Wrote %i bytes\n", bytes);
  writeImage(dest, bytes, paletteBuffer);
  return dest;
}

#define WINDOW_WIDTH 800
#define WINDOW_HEIGH 400

static inline uint8_t VGA6To8(uint8_t v) { return (v * 255) / 63; }

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

static void renderImage(SDL_Renderer *renderer, const uint8_t *imgData,
                        const uint8_t *paletteBuffer) {
  for (int x = 0; x < 320; x++) {
    for (int y = 0; y < 200; y++) {
      uint8_t paletteIdx = *(imgData + (320 * y) + x);

      uint8_t r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
      uint8_t g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
      uint8_t b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect = {.x = x * 2, .y = y * 2, .w = 2, .h = 2};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

static void writeImage(const uint8_t *imgData, size_t imgDataSize,
                       uint8_t *paletteBuffer) {
  printf("test write image\n");
  SDL_Event event;
  SDL_Renderer *renderer;
  SDL_Window *window;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGH, 0, &window,
                              &renderer);
  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
  SDL_RenderClear(renderer);

  renderImage(renderer, imgData, paletteBuffer);
  renderPalette(renderer, paletteBuffer, 640, 0);
  SDL_RenderPresent(renderer);
  while (1) {
    if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
      break;
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
