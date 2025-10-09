#pragma once
#include <stddef.h>
#include <stdint.h>

#define PALETTE_SIZE_256_6_RGB_VGA 768

typedef struct {
  uint8_t *data;
  size_t imageSize;
  uint8_t *palette;
  size_t paletteSize;
} CPSImage;

void CPSImageRelease(CPSImage *image);
int CPSImageFromBuffer(CPSImage *image, const uint8_t *buffer,
                       size_t bufferSize);
