#include "format_lcw.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// from https://github.com/OpenDUNE/OpenDUNE/blob/master/src/codec/format80.c
static uint16_t doDecompress(uint8_t *inBuf, uint32_t inLen, uint8_t *outBuf,
                             uint32_t outLen) {
  const uint8_t *start = outBuf;
  uint8_t *end = outBuf + outLen;

  while (outBuf != end) {
    uint8_t cmd;
    uint16_t size;
    uint16_t offset;

    cmd = *inBuf++;

    if (cmd == 0x80) {
      /* Exit */
      break;

    } else if ((cmd & 0x80) == 0) {
      /* Short move, relative */
      size = (cmd >> 4) + 3;
      if (size > end - outBuf)
        size = (uint16_t)(end - outBuf);

      offset = ((cmd & 0xF) << 8) + (*inBuf++);

      /* This decoder assumes memcpy. As some platforms implement memcpy as
       * memmove, this is much safer */
      for (; size > 0; size--) {
        if (outBuf - offset < start) {
          // printf("%X < %X\n", outBuf - offset, start);
          continue;
          // assert(outBuf - offset >= start);
        }

        *outBuf = *(outBuf - offset);
        outBuf++;
      }

    } else if (cmd == 0xFE) {
      /* Long set */
      size = *inBuf++;
      size += (*inBuf++) << 8;
      if (size > end - outBuf)
        size = (uint16_t)(end - outBuf);

      memset(outBuf, (*inBuf++), size);
      outBuf += size;

    } else if (cmd == 0xFF) {
      /* Long move, absolute */
      size = *inBuf++;
      size += (*inBuf++) << 8;
      if (size > end - outBuf)
        size = (uint16_t)(end - outBuf);

      offset = *inBuf++;
      offset += (*inBuf++) << 8;

      /* This decoder assumes memcpy. As some platforms implement memcpy as
       * memmove, this is much safer */
      for (; size > 0; size--)
        *outBuf++ = start[offset++];

    } else if ((cmd & 0x40) != 0) {
      /* Short move, absolute */
      size = (cmd & 0x3F) + 3;
      if (size > end - outBuf)
        size = (uint16_t)(end - outBuf);

      offset = *inBuf++;
      offset += (*inBuf++) << 8;

      /* This decoder assumes memcpy. As some platforms implement memcpy as
       * memmove, this is much safer */
      for (; size > 0; size--)
        *outBuf++ = start[offset++];

    } else {
      /* Short copy */
      size = cmd & 0x3F;
      if (size > end - outBuf)
        size = (uint16_t)(end - outBuf);

      /* This decoder assumes memcpy. As some platforms implement memcpy as
       * memmove, this is much safer */
      for (; size > 0; size--)
        *outBuf++ = *inBuf++;
    }
  }

  return (uint16_t)(outBuf - start);
}

ssize_t LCWDecompress(const uint8_t *source, size_t sourceSize, uint8_t *dest,
                      size_t destSize) {
  int written = doDecompress((uint8_t *)source, sourceSize, dest, destSize);
  printf("LCWDecompress.doDecompress wrote %i bytes destSize=%zi\n", written,
         destSize);
  return destSize;
}
