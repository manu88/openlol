#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint8_t numPalettes[1];
} BlockPalettePosTable;

typedef struct {
  uint8_t backdropWallPalettes[16];
} PosPaletteTables;

typedef struct {
  uint8_t rawData[4][8];
} Block;

#define VCN_PALETTE_TABLE_SIZE 8
#define VCN_PALETTE_BUFFER_SIZE 384
typedef struct {
  uint16_t nbBlocks;

  BlockPalettePosTable *blocksPalettePosTable; // array size = nbBlocks
  PosPaletteTables *posPaletteTables; // array size  VCN_PALETTE_TABLE_SIZE

  uint8_t *palette; // array size = VCN_PALETTE_BUFFER_SIZE 3*128

  Block *blocks; // array size = nbBlocks

  // freed by VCNDataRelease
  uint8_t *originalBuffer;
} VCNData;

void VCNDataRelease(VCNData *vcnData);
int VCNDataFromLCWBuffer(VCNData *vcnData, const uint8_t *buffer, size_t size);
