#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint8_t backdropWallPalettes[16];
} PosPaletteTables;

typedef struct {
  uint8_t rawData[32];
} VCNBlock;

#define VCN_PALETTE_TABLE_SIZE 8
#define VCN_PALETTE_BUFFER_SIZE 384
typedef struct {
  uint16_t nbBlocks;

  uint8_t *blocksPalettePosTable;     // array size = nbBlocks
  PosPaletteTables *posPaletteTables; // array size  VCN_PALETTE_TABLE_SIZE

  uint8_t *palette; // array size = VCN_PALETTE_BUFFER_SIZE 3*128

  VCNBlock *blocks; // array size = nbBlocks

  // freed by VCNDataRelease
  uint8_t *originalBuffer;
} VCNHandle;

void VCNHandleRelease(VCNHandle *handle);
int VCNHandleFromLCWBuffer(VCNHandle *handle, const uint8_t *buffer,
                           size_t size);
