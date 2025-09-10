#include "format_wll.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void WllHandleRelease(WllHandle *handle) { free(handle->originalData); }

int WllHandleFromBuffer(WllHandle *handle, uint8_t *buffer, size_t size) {
  handle->originalData = buffer;
  handle->dataSize = size - 14;
  handle->dataStart = handle->originalData + 14;
  handle->numEntries = (handle->dataSize) / 12;
  return 1;
}

uint16_t WllHandleGetWallID(const WllHandle *handle,
                            uint16_t wallMappingIndex) {
  for (int i = 0; i < handle->numEntries; i++) {
    const uint16_t *entry = WllHandleGetIndex(handle, i);
    if (*entry == wallMappingIndex) {
      if (entry[1] == 0) {
        printf("warning: WllHandleGetWallID will return 0\n");
        return 1;
      }
      return entry[1];
    }
  }
  assert(0);
  return 0;
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
