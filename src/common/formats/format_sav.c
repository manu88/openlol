#include "format_sav.h"
#include "geometry.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize) {
  assert(handle);
  handle->buffer = buffer;
  handle->bufferSize = bufferSize;
  return 1;
}

#define CHARACTERS_OFFSET 0X41
#define GENERAL_OFFSET 0X249
#define INVENTORY_OFFSET 0X25F
#define AFTER_INVENTORY_OFFSET INVENTORY_OFFSET + INVENTORY_SIZE

int SAVHandleGetSlot(const SAVHandle *handle, SAVSlot *slot) {
  assert(slot);

  slot->header = (SAVHeader *)handle->buffer;
  uint8_t *charBuffer = handle->buffer + CHARACTERS_OFFSET;
  for (int i = 0; i < 4; i++) {
    slot->characters[i] = (SAVCharacter *)charBuffer;
    charBuffer += SAVCharacterSize;
  }

  slot->general = (SAVGeneral *)(handle->buffer + GENERAL_OFFSET);

  slot->inventory = (int16_t *)(handle->buffer + INVENTORY_OFFSET);
  return 1;
}
