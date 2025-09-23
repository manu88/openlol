#include "format_40.h"
#include <assert.h>
#include <stdint.h>

void Format40Decode(const uint8_t *src, size_t srcSize, uint8_t *dst) {
  uint16_t cmd;
  uint16_t count;
  assert(src[srcSize - 3] == 0X80);
  assert(src[srcSize - 2] == 0X0);
  assert(src[srcSize - 1] == 0X0);

  for (;;) {
    cmd = *src++; /* 8 bit command code */

    if (cmd == 0) {
      /* XOR with value */
      for (count = *src++; count > 0; count--) {
        *dst++ ^= *src;
      }
      src++;
    } else if ((cmd & 0x80) == 0) {
      /* XOR with string */
      for (count = cmd; count > 0; count--) {
        *dst++ ^= *src++;
      }
    } else if (cmd != 0x80) {
      /* skip bytes */
      dst += (cmd & 0x7F);
    } else {
      /* last byte was 0x80 : read 16 bit value */
      cmd = *src++;
      cmd += (*src++) << 8;

      if (cmd == 0)
        break; /* 0x80 0x00 0x00 => exit code */

      if ((cmd & 0x8000) == 0) {
        /* skip bytes */
        dst += cmd;
      } else if ((cmd & 0x4000) == 0) {
        /* XOR with string */
        for (count = cmd & 0x3FFF; count > 0; count--) {
          *dst++ ^= *src++;
        }
      } else {
        /* XOR with value */
        for (count = cmd & 0x3FFF; count > 0; count--) {
          *dst++ ^= *src;
        }
        src++;
      }
    }
  }
}
