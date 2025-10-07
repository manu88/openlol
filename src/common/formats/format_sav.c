#include "format_sav.h"
#include "geometry.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define CHARACTERS_OFFSET 0X41
#define GENERAL_OFFSET 0X249
#define INVENTORY_OFFSET 0X25F
#define AFTER_INVENTORY_OFFSET INVENTORY_OFFSET + INVENTORY_SIZE

static int getSlot(const SAVHandle *handle, SAVSlot *slot) {
  assert(slot);

  slot->header = (SAVHeader *)handle->buffer;
  uint8_t *charBuffer = handle->buffer + CHARACTERS_OFFSET;
  for (int i = 0; i < 4; i++) {
    slot->characters[i] = (SAVCharacter *)charBuffer;
    charBuffer += SAVCharacterSize;
  }

  slot->general = (SAVGeneral *)(handle->buffer + GENERAL_OFFSET);
  slot->inventory = (uint16_t *)(handle->buffer + INVENTORY_OFFSET);
  return 1;
}

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize) {
  assert(handle);
  handle->buffer = buffer;
  handle->bufferSize = bufferSize;
  return getSlot(handle, &handle->slot);
}

static SAVGeneral _general = {
    .currentLevel = 1, .currentDirection = North, .currentBlock = 0X24D};

static uint16_t defaultInventory[INVENTORY_SIZE] = {0X0A, 0X0D, 0X12};

void SAVHandleGetNewGame(SAVHandle *handle) {
  assert(handle);
  handle->slot.general = &_general;
  handle->slot.inventory = defaultInventory;
}
