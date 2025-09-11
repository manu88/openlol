#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define DECORATION_NUM_ENTRIES 10
#define DECORATION_EMPTY_INDEX (uint16_t)0XFFFF

#define DatDecorationFlag_Flip_Bit

typedef enum {
  DatDecorationFlags_Mirror = 1 << 0,
} DatDecorationFlags;

typedef struct {
  uint16_t shapeIndex[DECORATION_NUM_ENTRIES];
  uint8_t scaleFlag[DECORATION_NUM_ENTRIES];
  int16_t shapeX[DECORATION_NUM_ENTRIES];
  int16_t shapeY[DECORATION_NUM_ENTRIES];
  int8_t next;
  uint8_t flags;
} DatDecoration;

typedef struct {
  uint16_t nbrDecorations;

  DatDecoration *datDecoration; // array size = nbrDecorations

  // freed by DatHandleRelease
  uint8_t *originalBuffer;
} DatHandle;

void DatHandleRelease(DatHandle *handle);
int DatHandleFromBuffer(DatHandle *handle, uint8_t *buffer, size_t size);
void DatHandlePrint(const DatHandle *handle);
