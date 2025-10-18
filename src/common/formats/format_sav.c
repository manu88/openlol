#include "format_sav.h"
#include "geometry.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define CHARACTERS_OFFSET 0X4C
#define GENERAL_OFFSET 0X254
#define INVENTORY_OFFSET 0X26A
#define ALL_OBJECTS_OFFSET 0X487
#define GENERAL2_OFFSET 0X460

static int getSlot(const SAVHandle *handle, SAVSlot *slot) {
  assert(slot);

  slot->header = (SAVHeader *)handle->buffer;
  uint8_t *charBuffer = handle->buffer + CHARACTERS_OFFSET;
  for (int i = 0; i < 4; i++) {
    slot->characters[i] = (SAVCharacter *)charBuffer;
    charBuffer += SAVCharacterSize;
  }

  slot->general = (SAVGeneral *)(handle->buffer + GENERAL_OFFSET);
  slot->general2 = (SAVGeneral2 *)(handle->buffer + GENERAL2_OFFSET);
  slot->inventory = (uint16_t *)(handle->buffer + INVENTORY_OFFSET);
  slot->gameObjects = (GameObject *)(handle->buffer + ALL_OBJECTS_OFFSET);
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
