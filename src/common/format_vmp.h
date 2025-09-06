#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  // used for drawing the blocks where the walls meet the ceilings/floors.
  uint8_t limit : 1;

  // 1 if the block graphics should be x-flipped, otherwise 0.
  uint8_t flipped : 1;

  // A 14-bit block index selecting a block in the vcn file.
  uint16_t blockIndex : 14;
} VMPTile;

typedef struct {

  uint16_t nbrOfBlocks;
  const VMPTile *tiles; // array size is nbrOfBlocks

  // freed by VMPDataRelease
  uint16_t *originalBuffer;
} VMPData;

void VMPDataRelease(VMPData *data);
int VMPDataFromLCWBuffer(VMPData *data, const uint8_t *buffer, size_t size);
void VMPDataTest(const VMPData *data);
