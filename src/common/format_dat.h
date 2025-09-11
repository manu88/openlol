#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t shapeIndex[10];
  uint8_t scaleFlag[10];
  int16_t shapeX[10];
  int16_t shapeY[10];
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
