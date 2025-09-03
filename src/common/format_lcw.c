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

// from https://moddingwiki.shikadi.net/wiki/Westwood_LCW
ssize_t LCWCompress(void const *input, void *output, unsigned long size) {
  // Decide if we are going to do relative offsets for 3 and 5 byte commands
  int relative = size > UINT16_MAX;

  if (!size || !input || !output) {
    return 0;
  }

  const uint8_t *getp = (const uint8_t *)input;
  uint8_t *putp = (uint8_t *)output;
  const uint8_t *getstart = getp;
  const uint8_t *getend = getp + size;
  uint8_t *putstart = putp;

  // relative LCW starts with 0 as flag to decoder.
  // this is only used by later games for decoding hi-color vqa files
  if (relative) {
    *putp++ = 0;
  }

  // Implementations that properly conform to the WestWood encoder should
  // write a starting cmd1. Its important for using the offset copy commands
  // to do more efficient RLE in some cases than the cmd4.

  // we also set bool to flag that we have an on going cmd1.
  uint8_t *cmd_onep = putp;
  *putp++ = 0x81;
  *putp++ = *getp++;
  int cmd_one = 1;

  // Compress data until we reach end of input buffer.
  while (getp < getend) {
    // Is RLE encode (4bytes) worth evaluating?
    if (getend - getp > 64 && *getp == *(getp + 64)) {
      // RLE run length is encoded as a short so max is UINT16_MAX
      const uint8_t *rlemax =
          (getend - getp) < UINT16_MAX ? getend : getp + UINT16_MAX;
      const uint8_t *rlep;

      for (rlep = getp + 1; *rlep == *getp && rlep < rlemax; ++rlep)
        ;

      uint16_t run_length = rlep - getp;

      // If run length is long enough, write the command and start loop again
      if (run_length >= 0x41) {
        // write 4byte command 0b11111110
        cmd_one = 0;
        *putp++ = 0xFE;
        *putp++ = run_length;
        *putp++ = run_length >> 8;
        *putp++ = *getp;
        getp = rlep;
        continue;
      }
    }

    // current block size for an offset copy
    int block_size = 0;
    const uint8_t *offstart;

    // Set where we start looking for matching runs.
    if (relative) {
      offstart = (getp - getstart) < UINT16_MAX ? getstart : getp - UINT16_MAX;
    } else {
      offstart = getstart;
    }

    // Look for matching runs
    const uint8_t *offchk = offstart;
    const uint8_t *offsetp = getp;
    while (offchk < getp) {
      // Move offchk to next matching position
      while (offchk < getp && *offchk != *getp) {
        ++offchk;
      }

      // If the checking pointer has reached current pos, break
      if (offchk >= getp) {
        break;
      }

      // find out how long the run of matches goes for
      int i;
      for (i = 1; &getp[i] < getend; ++i) {
        if (offchk[i] != getp[i]) {
          break;
        }
      }

      if (i >= block_size) {
        block_size = i;
        offsetp = offchk;
      }

      ++offchk;
    }

    // decide what encoding to use for current run
    // If its less than 2 bytes long, we store as is with cmd1
    if (block_size <= 2) {
      // short copy 0b10??????
      // check we have an existing 1 byte command and if its value is still
      // small enough to handle additional bytes
      // start a new command if current one doesn't have space or we don't
      // have one to continue
      if (cmd_one && *cmd_onep < 0xBF) {
        // increment command value
        ++*cmd_onep;
        *putp++ = *getp++;
      } else {
        cmd_onep = putp;
        *putp++ = 0x81;
        *putp++ = *getp++;
        cmd_one = 1;
      }
      // Otherwise we need to decide what relative copy command is most
      // efficient
    } else {
      uint16_t offset;
      uint16_t rel_offset = getp - offsetp;
      if (block_size > 0xA || ((rel_offset) > 0xFFF)) {
        // write 5 byte command 0b11111111
        if (block_size > 0x40) {
          *putp++ = 0xFF;
          *putp++ = block_size;
          *putp++ = block_size >> 8;
          // write 3 byte command 0b11??????
        } else {
          *putp++ = (block_size - 3) | 0xC0;
        }

        offset = relative ? rel_offset : offsetp - getstart;
        // write 2 byte command? 0b0???????
      } else {
        offset = rel_offset << 8 | (16 * (block_size - 3) + (rel_offset >> 8));
      }
      *putp++ = offset;
      *putp++ = offset >> 8;
      getp += block_size;
      cmd_one = 0;
    }
  }

  // write final 0x80, basically an empty cmd1 to signal the end of the stream.
  *putp++ = 0x80;

  // return the size of the compressed data.
  return putp - putstart;
}
