#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t wallMappingIndex;
  uint16_t wallType;
  uint16_t decorationId;

  uint16_t unknown1;
  uint16_t unknown2;
  uint16_t unknown3;
} WllWallMapping;

typedef struct {

  size_t numEntries;
  uint8_t *dataStart;

  uint8_t *originalData;
  size_t dataSize;
} WllHandle;

void WllHandleRelease(WllHandle *handle);
int WllHandleFromBuffer(WllHandle *handle, uint8_t *buffer, size_t size);
uint16_t *WllHandleGetIndex(const WllHandle *handle, size_t index);

const WllWallMapping* WllHandleGetWallMapping(const WllHandle *handle,
                              uint16_t wallMappingIndex);

uint16_t WllHandleGetWallType(const WllHandle *handle,
                              uint16_t wallMappingIndex);
void WLLHandlePrint(const WllHandle *handle);
