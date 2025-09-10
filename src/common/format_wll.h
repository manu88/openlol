#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t wallId;
} SomeData;

typedef struct {

  size_t numEntries;
  uint8_t *dataStart;

  uint8_t *originalData;
  size_t dataSize;
} WllHandle;

void WllHandleRelease(WllHandle *handle);
int WllHandleFromBuffer(WllHandle *handle, uint8_t *buffer, size_t size);
uint16_t *WllHandleGetIndex(const WllHandle *handle, size_t index);
uint16_t WllHandleGetWallID(const WllHandle *handle, uint16_t wallMappingIndex);
void WLLHandlePrint(const WllHandle *handle);
