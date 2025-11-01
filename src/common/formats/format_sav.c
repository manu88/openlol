#include "format_sav.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
  const uint8_t *b = (uint8_t *)(handle->buffer + GENERAL_OFFSET);
  int off = 0;

  slot->general.currentBlock = *(uint16_t *)(b + off);
  off += 2;

  slot->general.posX = (uint16_t)*(b + off);
  off += 2;

  slot->general.posY = *(uint16_t *)(b + off);
  off += 2;

  slot->general.updateFlags = *(uint16_t *)(b + off);
  off += 2;

  slot->general.scriptDir = b[off];
  off += 1;

  slot->general.selectedSpell = b[off];
  off += 1;

  off += 2; // skip

  slot->general.sceneDefaultUpdate = b[off];
  off += 1;

  slot->general.compassBroken = b[off];
  off += 1;

  slot->general.drainMagic = b[off];
  off += 1;

  slot->general.currentDirection = *(uint16_t *)(b + off);
  off += 2;

  slot->general.compassDirection = *(uint16_t *)(b + off);
  off += 2;

  slot->general.selectedChar = b[off];
  off += 1;

  off += 1; // skip

  slot->general.currentLevel = *(b + off);
  off += 1;

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    slot->general.inventory[i] = *(uint16_t *)(b + off);
    off += 2;
  }

  slot->general.inventoryCurrentItem = *(uint16_t *)(b + off);
  off += 2;

  slot->general.itemIndexInHand = *(uint16_t *)(b + off);
  off += 2;

  slot->general.lastMouseRegion = *(uint16_t *)(b + off);
  off += 2;

  memset(slot->general.flags, 0, NUM_FLAGS * 2);
  for (int i = 0; i < NUM_FLAGS; i++) {
    slot->general.flags[i] = *(uint16_t *)(b + off);
    off += 2;
  }

  slot->general2 = (SAVGeneral2 *)(handle->buffer + GENERAL2_OFFSET);
  slot->gameObjects = (GameObject *)(handle->buffer + ALL_OBJECTS_OFFSET);
  return 1;
}

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize) {
  assert(handle);
  handle->buffer = buffer;
  handle->bufferSize = bufferSize;
  return getSlot(handle, &handle->slot);
}

void SAVHandleGetGameFlags(const SAVHandle *handle, uint8_t *gameFlags,
                           size_t numGameFlags) {
  for (int i = 0; i < NUM_FLAGS; i++) {
    for (uint8_t k = 0; k < 16; ++k) {
      if (handle->slot.general.flags[i] & (1 << k)) {
        uint8_t flag = ((i << 4) & 0xFFF0) | (k & 0x000F);
        uint16_t flagIndex = flag >> 3;
#if 0
        if (flagIndex >= numGameFlags) {
          printf("invalid flagIndex %i  at %i (%X) \n", flagIndex, i,
                 handle->slot.general->flags[i]);
        }
        assert(flagIndex >= 0 && flagIndex < numGameFlags);
#endif
        gameFlags[flagIndex] |= (1 << (flag & 7));
      }
    }
  }
}
