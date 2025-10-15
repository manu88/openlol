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
  const VMPCompressionHeader *header = (const VMPCompressionHeader *)buffer;

  uint8_t *uncompressedData = malloc(header->uncompressedSize);
  if (!uncompressedData) {
    return 0;
  }
  assert(LCWDecompress(buffer + VCNHEADER_SIZE, size - VCNHEADER_SIZE,
                       uncompressedData,
                       header->uncompressedSize) == header->uncompressedSize);
  handle->originalBuffer = (uint16_t *)uncompressedData;

  handle->nbrOfBlocks = *handle->originalBuffer;
  assert(handle->nbrOfBlocks <= UINT16_MAX);
  return 1;
}

int VMPHandleGetTile(const VMPHandle *handle, uint32_t index, VMPTile *tile) {
  assert(tile);
  if (index >= handle->nbrOfBlocks || index < 0) {
    printf("VMPHandleGetTile index %u>=%i\n", index, handle->nbrOfBlocks);
  }
  assert(index < handle->nbrOfBlocks);
  assert(index >= 0);

  uint16_t v = handle->originalBuffer[index + 1];
  tile->blockIndex = v & 0x3fff;
  tile->flipped = (v & 0x04000) == 0x04000;
  return 1;
}

void VMPHandlePrint(const VMPHandle *handle) {

  printf("got %hu VMP blocks\n", handle->nbrOfBlocks);
  for (int i = 0; i < handle->nbrOfBlocks; i++) {

    VMPTile tile = {0};
    VMPHandleGetTile(handle, i, &tile);

    printf("%i/%i: %i %X\n", i, handle->nbrOfBlocks, tile.flipped,
           tile.blockIndex);
  }
}
