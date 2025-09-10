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
  return 0;
}

uint16_t *WllHandleGetIndex(const WllHandle *handle, size_t index) {
  assert(handle);
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
