#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t shapeId;
  uint8_t enabled;
  uint8_t p;
  uint8_t pad[7];
  uint16_t stringId; // 2
} LegendEntry;

static_assert(sizeof(LegendEntry) == 12, "");
typedef struct {
  LegendEntry *entries;
  size_t numEntries;
} XXXHandle;

int XXXHandleFromBuffer(XXXHandle *handle, uint8_t *buffer, size_t size);
