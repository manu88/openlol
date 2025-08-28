#include "format_cps.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// https://moddingwiki.shikadi.net/wiki/Westwood_CPS_Format
// SHP https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(Lands_of_Lore)

uint8_t *paletteBuffer = NULL;
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static void printBits(size_t const size, void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  puts(" ");
}

static int ParseCPSBuffer(uint8_t *inBuf, uint32_t inLen, uint8_t *outBuf,
                          uint32_t outLen) {
  int version = 1;
  int count, i, color, pos, relpos;

  uint8_t *src = inBuf;
  uint8_t *dst = outBuf + 1; // one byte beyond the last written byte
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
        dst[i] = *(dst + i - relpos);
      }
    }

    dst += count;
  }

  return dst - outBuf;
}

uint8_t *TestCps(const uint8_t *buffer, size_t bufferSize) {

  const CPSFileHeader *file = (const CPSFileHeader *)buffer;
  assert(file->compressionType <= CPSCompressionType_LZW_LCW);
  printf("test CPS\n");
  printf("PAK buffer size %zi\n", bufferSize);
  printf("size %i\n", file->fileSize);
  printf("compressionType %i\n", file->compressionType);
  printf("uncompressedSize %i\n", file->uncompressedSize);
  printf("paletteSize %i\n", file->paletteSize);
  assert(file->uncompressedSize == 64000);
  assert(file->paletteSize == 0 ||
         file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA);

  uint8_t *dataBuffer = (uint8_t *)buffer + sizeof(CPSFileHeader);
  size_t dataSize = bufferSize - sizeof(CPSFileHeader);
  if (file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA) {
    paletteBuffer = (uint8_t *)buffer + sizeof(CPSFileHeader);
    dataBuffer += file->paletteSize;
    dataSize -= file->paletteSize;
  }
  uint8_t *dest = malloc(file->uncompressedSize);
  assert(dest);
  int bytes = ParseCPSBuffer(dataBuffer, dataSize, dest, 64000);
  printf("Wrote %i bytes\n", bytes);
  return dest;
}
