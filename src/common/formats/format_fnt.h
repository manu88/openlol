#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t maxWidth;
  uint8_t maxHeight;
  uint16_t numGlyphs;

  uint8_t *widthTable;
  uint8_t *heightTable;
  uint16_t *bitmapOffsets;

  uint8_t *buffer;
  size_t bufferSize;
} FNTHandle;

int FNTHandleFromBuffer(FNTHandle *handle, uint8_t *buffer, size_t bufferSize);

static inline uint8_t FNTHandleGetCharHeight(const FNTHandle *handle,
                                             uint16_t index) {
  return handle->heightTable[index] >> 8;
}

static inline uint8_t FNTHandleGetYOffset(const FNTHandle *handle,
                                          uint16_t index) {
  return handle->heightTable[index] & 0XFF;
}
