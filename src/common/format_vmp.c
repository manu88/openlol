#include "format_vmp.h"
#include "format_lcw.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define VCNHEADER_SIZE 10
typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
} VMPCompressionHeader;

void VMPHandleRelease(VMPHandle *handle) { free(handle->originalBuffer); }

int VMPHandleFromLCWBuffer(VMPHandle *handle, const uint8_t *buffer,
                           size_t size) {
  printf("size VMP file=%zi\n", size);
  const VMPCompressionHeader *header = (const VMPCompressionHeader *)buffer;
  printf("fileSize=%i comp=%i outSize=%i\n", header->fileSize,
         header->compressionType, header->uncompressedSize);

  uint8_t *uncompressedData = malloc(header->uncompressedSize);
  if (!uncompressedData) {
    return 0;
  }
  assert(LCWDecompress(buffer + VCNHEADER_SIZE, size - VCNHEADER_SIZE,
                       uncompressedData,
                       header->uncompressedSize) == header->uncompressedSize);
  handle->originalBuffer = (uint16_t *)uncompressedData;

  handle->nbrOfBlocks = *handle->originalBuffer;
  handle->tiles = (const VMPTile *)handle->originalBuffer + 1;
  return 1;
}

void VMPHandleTest(const VMPHandle *handle) {

  printf("got %hu VMP blocks\n", handle->nbrOfBlocks);
  assert(sizeof(VMPTile) == 2);
  for (int i = 0; i < handle->nbrOfBlocks; i++) {

    const VMPTile *tile = (const VMPTile *)handle->tiles + i;
    printf("%i/%i: %i %i %X\n", i, handle->nbrOfBlocks, tile->limit,
           tile->flipped, tile->blockIndex);
  }
}
