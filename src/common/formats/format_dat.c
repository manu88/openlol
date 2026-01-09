#include "format_dat.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void DatHandleRelease(DatHandle *handle) { free(handle->originalBuffer); }

int DatHandleFromBuffer(DatHandle *handle, uint8_t *buffer, size_t size) {
  handle->originalBuffer = buffer;

  handle->nbrDecorations = *(uint16_t *)(handle->originalBuffer);

  handle->datDecoration = (DatDecoration *)(handle->originalBuffer + 2);
  return 1;
}

void DatDecorationPrint(const DatDecoration *deco) {
  printf("next=%X flags=%X \n", deco->next, deco->flags);
  for (int i = 0; i < DECORATION_NUM_ENTRIES; i++) {
    if (deco->shapeIndex[i] == DECORATION_EMPTY_INDEX) {
      continue;
    }
    printf("\tindex=%i x=%X y=%X scale=%X\n", deco->shapeIndex[i],
           deco->shapeX[i], deco->shapeY[i], deco->scaleFlag[i]);
  }
}

void DatHandlePrint(const DatHandle *handle) {
  printf("nbrDecorations=%i\n", handle->nbrDecorations);
  for (int i = 0; i < handle->nbrDecorations; i++) {
    const DatDecoration *deco = handle->datDecoration + i;
    printf("Decoration %i: ", i);
    DatDecorationPrint(deco);
  }
}
