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
#define GETLONG(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16) | ((x)[3] << 24))
#define MAXSRCLEN (64 * 1024)    /* max length of crunched data */
#define MAXDESTLEN (1024 * 1024) /* max length of unpacked data */

#define EMPTYFILE (0)
#define OVERFLOW (-1)
#define UNKNOWNCOMP (-2)

/* compression mode 0 = no compression */
static int cps_copy(unsigned char *src, unsigned char *dest) {
  int len, x;

  /* get the length of the 'uncompressed' data */
  if ((len = GETLONG(src)) == 0)
    return EMPTYFILE;

  /* check for source overflow. we can't overflow the destination */
  if (len < 0 || len > (MAXSRCLEN - GETWORD(&src[4]) - 6))
    return OVERFLOW;

  /* skip to the start of data and do a direct copy from source to dest */
  src += GETWORD(&src[4]) + 6;
  for (x = len; x--;)
    *dest++ = *src++;

  return len;
}

/* compression mode 3 = run-length encoding */
static int cps_rle(unsigned char *src, unsigned char *dest) {
  int len, x;
  char rep;

  /* get the length of the 'uncompressed' data */
  if ((len = GETLONG(src)) == 0)
    return EMPTYFILE;

  /* check for destination overflow. */
  if (len < 0 || len > MAXDESTLEN)
    return OVERFLOW;

  /* skip to start of data */
  src += GETWORD(&src[4]) + 6;

  /* run while we have room left in output buffer */
  for (x = len; x > 0;) {
    int rlen = *src++;
    if (rlen < 128) {
      /* direct copy, no RLE */
      x -= rlen;
      if (x < 0)
        return OVERFLOW;
      while (rlen--)
        *dest++ = *src++;
    } else {
      /* repeated character RLE */
      if (rlen == 0) {
        rlen = GETWORD(src);
        src += 2;
      } else
        rlen = 256 - rlen;
      x -= rlen;
      if (x < 0)
        return OVERFLOW;
      rep = *src++;
      while (rlen--)
        *dest++ = rep;
    }
  }
  return len - x;
}

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
