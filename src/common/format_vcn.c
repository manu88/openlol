#include "format_vcn.h"
#include "bytes.h"
#include "format_lcw.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// https://web.archive.org/web/20180313235313/http://eob.wikispaces.com/eob.vcn#LoL

#define VCNHEADER_SIZE 10
typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
} VCNCompressionHeader;

void VCNHandleRelease(VCNHandle *handle) { free(handle->originalBuffer); }

int VCNHandleFromLCWBuffer(VCNHandle *handle, const uint8_t *buffer,
                           size_t size) {
  assert(handle);
  assert(buffer);
  printf("size VCN file=%zi\n", size);
  const VCNCompressionHeader *header = (const VCNCompressionHeader *)buffer;
  printf("fileSize=%i comp=%i outSize=%i\n", header->fileSize,
         header->compressionType, header->uncompressedSize);

  uint8_t *dest = malloc(header->uncompressedSize);
  if (!dest) {
    return 0;
  }
  assert(LCWDecompress(buffer + VCNHEADER_SIZE, size - VCNHEADER_SIZE, dest,
                       header->uncompressedSize) == header->uncompressedSize);

  handle->originalBuffer = dest;
  handle->nbBlocks = *(const uint16_t *)dest;
  dest += 2;

  handle->blocksPalettePosTable = dest;
  dest += handle->nbBlocks;

  handle->posPaletteTables = (PosPaletteTables *)dest;
  dest += 8 * sizeof(PosPaletteTables);

  uint8_t *palette = dest;
  dest += 3 * 128;

  VCNBlock *blocks = (VCNBlock *)dest;
  dest += handle->nbBlocks * sizeof(VCNBlock);

  handle->palette = palette;
  handle->blocks = blocks;
  return 1;
}
