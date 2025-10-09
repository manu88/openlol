#include "format_cps.h"
#include "format_lcw.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  CPSCompressionType_Uncompressed = 0,
  CPSCompressionType_LZW_12 = 1,
  CPSCompressionType_LZW_14 = 2,
  CPSCompressionType_LZW_RLE = 3,
  CPSCompressionType_LZW_LCW = 4,
} CPSCompressionType;

void CPSImageRelease(CPSImage *image) {
  free(image->data);
  free(image->palette);
}

typedef struct {
  uint16_t fileSize; // For compression method 0 and 4, this only counts the
                     // bytes behind this value, making it two bytes less than
                     // the actual file size.
  uint16_t compressionType;  // see CPSCompressionType
  uint32_t uncompressedSize; // For PC files, this should always be 64000, since
                             // the data contains a linear 8-bit 320×200 image.
  uint16_t paletteSize; // This can either be 0, for no palette, 768 for a PC
                        // 256-colour 6-bit RGB VGA palette,
} CPSFileHeader;

int CPSImageFromBuffer(CPSImage *image, const uint8_t *buffer,
                       size_t bufferSize) {
  CPSFileHeader *file = (CPSFileHeader *)buffer;
  assert(file->compressionType <= CPSCompressionType_LZW_LCW);
  file->fileSize += 2; // CPSCompressionType_LZW_LCW: must add 2 bytes
  assert(file->uncompressedSize == 64000);
  assert(file->paletteSize == 0 ||
         file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA);

  size_t dataSize = bufferSize - 10;

  uint8_t *dataBuffer = (uint8_t *)buffer + 10;

  uint8_t *paletteBuffer = dataBuffer;
  if (file->paletteSize == PALETTE_SIZE_256_6_RGB_VGA) {
    dataBuffer += file->paletteSize;
    dataSize -= file->paletteSize;
    image->palette = malloc(file->paletteSize);
    assert(image->palette);
    memcpy(image->palette, paletteBuffer, file->paletteSize);
  }
  image->paletteSize = file->paletteSize;
  image->data = malloc(file->uncompressedSize);
  memset(image->data, 0, file->uncompressedSize);
  image->imageSize = file->uncompressedSize;
  if (!image->data) {
    return 0;
  }

  int bytes =
      LCWDecompress(dataBuffer, dataSize, image->data, file->uncompressedSize);
  assert(bytes == file->uncompressedSize);
  return 1;
}
