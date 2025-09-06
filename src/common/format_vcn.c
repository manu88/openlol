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

static void printVCN(const VCNHandle *handle, int blockId) {
  printf("VCN: %i/%i blocks\n", blockId, handle->nbBlocks);

  const Block *blk = handle->blocks + blockId;
  uint8_t numPalette =
      (handle->blocksPalettePosTable + blockId)->numPalettes[0] / 16;
  printf("Num pal = %x\n", numPalette);
  assert(numPalette < VCN_PALETTE_TABLE_SIZE);
  uint8_t out[8][8][3] = {0};

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 4; x++) {
      uint8_t rawData = blk->rawData[x][y];
      uint8_t p0 = rawData >> 4;
      uint8_t p1 = rawData & 0X0F;

      uint8_t idx0 =
          handle->posPaletteTables[numPalette].backdropWallPalettes[p0];
      assert(idx0 < 128);
      uint8_t *colorPix0 = handle->palette + idx0;

      uint8_t idx1 =
          handle->posPaletteTables[numPalette].backdropWallPalettes[p1];
      assert(idx1 < 128);

      uint8_t *colorPix1 = handle->palette + idx1;
      out[x * 2][y][0] = *colorPix0;
      out[1 + x * 2][y][0] = *colorPix1;
    }
  }
}

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
  const uint16_t nbBlocks = *(const uint16_t *)dest;
  dest += 2;

  BlockPalettePosTable *blockPalettePosTable = (BlockPalettePosTable *)dest;
  dest += nbBlocks * sizeof(BlockPalettePosTable);

  PosPaletteTables *posPaletteTables = (PosPaletteTables *)dest;
  dest += 8 * sizeof(PosPaletteTables);

  uint8_t *palette = dest;
  dest += 3 * 128;

  Block *blocks = (Block *)dest;
  dest += nbBlocks * sizeof(Block);

  handle->nbBlocks = nbBlocks;
  handle->blocksPalettePosTable = blockPalettePosTable;
  handle->posPaletteTables = posPaletteTables;
  handle->palette = palette;
  handle->blocks = blocks;
  return 1;
}
