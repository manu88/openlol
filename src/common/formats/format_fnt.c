#include "format_fnt.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int FNTHandleFromBuffer(FNTHandle *handle, uint8_t *buffer, size_t bufferSize) {
  handle->buffer = buffer;
  handle->bufferSize = bufferSize;

  uint8_t *data = buffer;
  const uint16_t fontSig = *(const uint16_t *)(data + 2);

  assert(fontSig == 0X0500); // probably ok if 0X0002

  const uint16_t descOffset = *(const uint16_t *)(data + 4);

  handle->maxWidth = data[descOffset + 5];
  handle->maxHeight = data[descOffset + 4];
  handle->numGlyphs = data[descOffset + 3] + 1;

  handle->bitmapOffsets = (uint16_t *)(data + *(const uint16_t *)(data + 6));
  handle->widthTable = data + *(const uint16_t *)(data + 8);
  handle->heightTable = data + *(const uint16_t *)(data + 12);

  for (int i = 0; i < handle->numGlyphs; ++i) {
    handle->bitmapOffsets[i] = *(const uint16_t *)(&handle->bitmapOffsets[i]);
  }

  return 1;
}
