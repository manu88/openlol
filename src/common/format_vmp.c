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

void VMPDataRelease(VMPData *vpmData) { free(vpmData->originalBuffer); }

int VMPDataFromLCWBuffer(VMPData *data, const uint8_t *buffer, size_t size) {
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
  data->originalBuffer = (uint16_t *)uncompressedData;

  data->nbrOfBlocks = *data->originalBuffer;
  data->tiles = (const VMPTile *)data->originalBuffer + 1;
  return 1;
}

void VMPDataTest(const VMPData *data) {

  printf("got %hu VMP blocks\n", data->nbrOfBlocks);
  assert(sizeof(VMPTile) == 2);
  for (int i = 0; i < data->nbrOfBlocks; i++) {

    const VMPTile *tile = (const VMPTile *)data->tiles + i;
    printf("%i/%i: %i %i %X\n", i, data->nbrOfBlocks, tile->limit,
           tile->flipped, tile->blockIndex);
  }
}
