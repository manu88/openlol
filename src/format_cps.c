#include "format_cps.h"
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

// https://moddingwiki.shikadi.net/wiki/Westwood_CPS_Format
// SHP https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(Lands_of_Lore)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

int ParseCPSBuffer(uint8_t *inBuf, uint32_t inLen, uint8_t *outBuf,
                   uint32_t outLen) {
  int version = 1;
  int count, i, color, pos, relpos;

  uint8_t *src = inBuf;
  uint8_t *dst = outBuf; // one byte beyond the last written byte
  uint8_t *outEnd = dst + outLen;

  if (src[0] == 0) {
    version = 2;
    ++src;
  }

  while (src < inBuf + inLen && dst < outEnd && src[0] != 0x80) {
    int out_remain = (int)(outEnd - dst);
    if (src[0] == 0xff) { // 0b11111111
      count = src[1] | (src[2] << 8);
      pos = src[3] | (src[4] << 8);
      src += 5;
      count = MIN(count, out_remain);

      if (version == 1) {
        for (i = 0; i < count; ++i)
          dst[i] = outBuf[i + pos];
      } else {
        for (i = 0; i < count; ++i)
          dst[i] = *(dst + i - pos);
      }
    } else if (src[0] == 0xfe) { // 0b11111110
      count = src[1] | (src[2] << 8);
      color = src[3];
      src += 4;
      count = MIN(count, out_remain);

      memset(dst, color, count);
    } else if (src[0] >= 0xc0) { // 0b11??????
      count = (src[0] & 0x3f) + 3;
      pos = src[1] | (src[2] << 8);
      src += 3;
      count = MIN(count, out_remain);

      if (version == 1) {
        for (i = 0; i < count; ++i)
          dst[i] = outBuf[i + pos];
      } else {
        for (i = 0; i < count; ++i)
          dst[i] = *(dst + i - pos);
      }
    } else if (src[0] >= 0x80) { // 0b10??????
      count = src[0] & 0x3f;
      ++src;
      count = MIN(count, out_remain);

      memcpy(dst, src, count);
      src += count;
    } else { // 0b0???????
      count = ((src[0] & 0x70) >> 4) + 3;
      relpos = ((src[0] & 0x0f) << 8) | src[1];
      src += 2;
      count = MIN(count, out_remain);

      for (i = 0; i < count; ++i) {
        if (dst + i - relpos < dst) {
          continue;
        }
        dst[i] = *(dst + i - relpos);
      }
    }

    dst += count;
  }

  return dst - outBuf;
}

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
  int bytes =
      ParseCPSBuffer(dataBuffer, dataSize, dest, file->uncompressedSize);
  printf("Wrote %i bytes\n", bytes);
  writeImage(dest, bytes, paletteBuffer);
  return dest;
}

#define WINDOW_WIDTH 320
#define WINDOW_HEIGH 200

static uint8_t VGA6To8(uint8_t v) { return (v * 255) / 63; }
static uint8_t VGA8To8(uint8_t v) { return (v); }

static void renderPalette(SDL_Renderer *renderer,
                          const uint8_t *paletteBuffer) {
  for (int i = 0; i < 256; i++) {
    int x = i % 16;
    int y = i / 16;

    uint8_t r = VGA6To8(paletteBuffer[(i * 3) + 0]);
    uint8_t g = VGA6To8(paletteBuffer[(i * 3) + 1]);
    uint8_t b = VGA6To8(paletteBuffer[(i * 3) + 2]);
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {.x = x * 10, .y = y * 10, .w = 10, .h = 10};
    SDL_RenderFillRect(renderer, &rect);
  }
}

static void renderImage(SDL_Renderer *renderer, const uint8_t *imgData,
                        const uint8_t *paletteBuffer) {
  for (int x = 0; x < WINDOW_WIDTH; x++) {
    for (int y = 0; y < WINDOW_HEIGH; y++) {
      uint8_t paletteIdx = *(imgData + (WINDOW_WIDTH * y) + x);

      uint8_t r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
      uint8_t g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
      uint8_t b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_RenderDrawPoint(renderer, x, y);
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
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  renderImage(renderer, imgData, paletteBuffer);
  // renderPalette(renderer, paletteBuffer);
  SDL_RenderPresent(renderer);
  while (1) {
    if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
      break;
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
