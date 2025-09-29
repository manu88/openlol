#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  // used for drawing the blocks where the walls meet the ceilings/floors.
  // uint8_t limit;

  // 1 if the block graphics should be x-flipped, otherwise 0.
  uint8_t flipped;

  // A 14-bit block index selecting a block in the vcn file.
  uint16_t blockIndex;
} VMPTile;

typedef struct {
  uint16_t nbrOfBlocks;

  // freed by VMPDataRelease
  uint16_t *originalBuffer;
} VMPHandle;

void VMPHandleRelease(VMPHandle *handle);
int VMPHandleFromLCWBuffer(VMPHandle *handle, const uint8_t *buffer,
                           size_t size);
void VMPHandlePrint(const VMPHandle *handle);
int VMPHandleGetTile(const VMPHandle *handle, uint32_t index, VMPTile *tile);
