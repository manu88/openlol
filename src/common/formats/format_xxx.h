#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t x;        // 3
  uint16_t y;        // 4
  uint16_t shapeId;  // 5
  uint16_t p3;       // 0
  uint16_t p4;       // 1
  uint16_t stringId; // 2
} LegendEntry;

typedef struct {
  LegendEntry *entries;
  size_t numEntries;
} XXXHandle;

int XXXHandleFromBuffer(XXXHandle *handle, uint8_t *buffer, size_t size);
