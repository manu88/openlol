#include "format_wll.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void WllHandleRelease(WllHandle *handle) { free(handle->originalData); }

int WllHandleFromBuffer(WllHandle *handle, uint8_t *buffer, size_t size) {
  handle->originalData = buffer;
  handle->dataSize = size - 2 - sizeof(WllWallMapping);
  handle->dataStart = handle->originalData + 2 + sizeof(WllWallMapping);
  handle->numEntries = (handle->dataSize) / sizeof(WllWallMapping);
  return 1;
}

const WllWallMapping *WllHandleGetWallMapping(const WllHandle *handle,
                                              uint16_t wallMappingIndex) {
  for (int i = 0; i < handle->numEntries; i++) {
    const WllWallMapping *entry =
        (const WllWallMapping *)WllHandleGetIndex(handle, i);
    if (entry->wallMappingIndex == wallMappingIndex) {
      return entry;
    }
  }
  return NULL;
}

uint16_t WllHandleGetWallType(const WllHandle *handle,
                              uint16_t wallMappingIndex) {

  const WllWallMapping *entry =
      WllHandleGetWallMapping(handle, wallMappingIndex);
  if (entry == NULL) {
    printf("warning: WllHandleGetWallID did not find any value for %u, "
           "returning 1\n",
           wallMappingIndex);
    return 1;
  }
  if (entry->wallType == 0) {
    printf("warning: WllHandleGetWallID will return 0 for wmi %u\n",
           wallMappingIndex);
    return 1;
  }
  return entry->wallType;
}

uint16_t *WllHandleGetIndex(const WllHandle *handle, size_t index) {
  assert(handle);
  if (index >= handle->numEntries) {
    printf("WllHandleGetIndex: index %zu >= %zu\n", index, handle->numEntries);
  }
  assert(index < handle->numEntries);
  return (uint16_t *)(handle->dataStart + index * 12);
}

void WLLHandlePrint(const WllHandle *handle) {
  printf("Got %zu entries\n", handle->numEntries);
  for (int i = 0; i < handle->numEntries; i++) {
    const uint16_t *entry = WllHandleGetIndex(handle, i);
    for (int i = 0; i < 6; i++) {
      printf("0X%04X ", entry[i]);
    }
    printf("\n");
  }
}
