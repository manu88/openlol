#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum __attribute__((__packed__)) {
  WallSpecialType_None = 0,
  WallSpecialType_WallShape = 1,
  WallSpecialType_LeverOn = 2,
  WallSpecialType_LeverOff = 3,
  WallSpecialType_OnlyScript = 4,
  WallSpecialType_DoorSwitch = 5,
  WallSpecialType_Niche = 6,
} WallSpecialType;

static_assert(sizeof(WallSpecialType) == 1, "WallSpecialType size should be 1");

typedef struct {
  uint16_t wallMappingIndex;
  uint16_t wallType;
  uint16_t decorationId;

  WallSpecialType specialType;
  uint8_t _unused;
  uint16_t flags;
  uint16_t automapData;
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

const WllWallMapping *WllHandleGetWallMapping(const WllHandle *handle,
                                              uint16_t wallMappingIndex);

uint16_t WllHandleGetWallType(const WllHandle *handle,
                              uint16_t wallMappingIndex);
void WLLHandlePrint(const WllHandle *handle);
