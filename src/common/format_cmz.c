/* decompression code from https://www.kyzer.me.uk/pack/uncps.c */
/* Decrunches 'Eye of the Beholder' CPS files. */
/* (C) 2000 Kyzer/CSG */
#include "format_cmz.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GETWORD(x) ((x)[0] | ((x)[1] << 8))
#define MAXSRCLEN (64 * 1024)    /* max length of crunched data */
#define MAXDESTLEN (1024 * 1024) /* max length of unpacked data */

#define OVERFLOW (-1)

/* compression mode 4 = LZ77 style compression */
static int cps_lz77(const unsigned char *src, unsigned char *dest) {
  unsigned char *s = (unsigned char *)src, *d = dest, *rep;
  unsigned int len, code;

  while ((s - src) < MAXSRCLEN) {
    code = *s++;
    if (code == 0xFE) {
      /* mini-RLE */
      len = GETWORD(s);
      s += 2;
      code = *s++;
      if ((d + len - dest) > MAXDESTLEN)
        return OVERFLOW;
      while (len--)
        *d++ = code;
    } else {
      if (code >= 0xC0) {
        if (code == 0xFF) {
          len = GETWORD(s);
          s += 2;
        } else {
          len = (code & 0x3F) + 3;
        }
        rep = dest + GETWORD(s);
        s += 2;
      } else if (code >= 0x80) {
        if (code == 0x80)
          break;
        len = code & 0x3F;
        rep = s;
        s += len;
      } else /* code < 0x80 */ {
        len = (code >> 4) + 3;
        rep = d - (((code & 0x0F) << 8) | *s++);
      }

      if ((d + len - dest) > MAXDESTLEN)
        return OVERFLOW;
      while (len--)
        *d++ = *rep++;
    }
  }
  return d - dest;
}

#define CMZHEADER_SIZE 10
typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
} CMZHeader;

uint8_t *CMZDecompress(const uint8_t *inBuffer, size_t inBufferSize,
                       size_t *outBufferSize) {
  assert(inBuffer);
  assert(outBufferSize);
  const CMZHeader *header = (const CMZHeader *)inBuffer;

  // only support this right now, handling other types is not
  // complicated and already implemented in the original uncps file.
  assert(header->compressionType == 4);
  *outBufferSize = header->uncompressedSize;
  uint8_t *dest = malloc(*outBufferSize);
  if (!dest) {
    return NULL;
  }
  assert(cps_lz77(inBuffer + CMZHEADER_SIZE, dest) == *outBufferSize);

  return dest;
}

void MazeHandleRelease(MazeHandle *mazeHandle) { free(mazeHandle->maze); }

int MazeHandleFromBuffer(MazeHandle *mazeHandle, const uint8_t *inBuffer,
                         size_t inBufferSize) {
  assert(inBuffer);

  const CMZHeader *header = (const CMZHeader *)inBuffer;

  // only support this right now, handling other types is not
  // complicated and already implemented in the original uncps file.
  assert(header->compressionType == 4);

  uint8_t *dest = malloc(header->uncompressedSize);
  if (!dest) {
    return 0;
  }
  assert(cps_lz77(inBuffer + CMZHEADER_SIZE, dest) == header->uncompressedSize);
  mazeHandle->maze = (Maze *)dest;
  return 1;
}
