#include "format_sav.h"
#include <_static_assert.h>
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

static void getCharacter(SAVCharacter *c, uint8_t *b) {
  int off = 0;

  c->flags = *(uint16_t *)(b + off);
  off += 2;

  memcpy(c->name, b + off, 11);

  off += 11;

  c->raceClassSex = b[off];
  off += 1;

  c->id = *(uint16_t *)(b + off);
  off += 2;

  c->currentFaceFrame = b[off];
  off += 1;

  c->tempFaceFrame = b[off];
  off += 1;

  c->screamSfx = b[off];
  off += 1;

  off += 4; // skip

  for (int i = 0; i < 8; i++) {
    c->itemsMight[i] = *(uint16_t *)(b + off);
    off += 2;
  }

  for (int i = 0; i < 8; i++) {
    c->itemsProtection[i] = *(uint16_t *)(b + off);
    off += 2;
  }

  c->itemProtection = *(uint16_t *)(b + off);
  off += 2;

  c->hitPointsCur = *(uint16_t *)(b + off);
  off += 2;

  c->hitPointsMax = *(uint16_t *)(b + off);
  off += 2;

  c->magicPointsCur = *(uint16_t *)(b + off);
  off += 2;

  c->magicPointsMax = *(uint16_t *)(b + off);
  off += 2;

  c->unknown_41 = b[off];
  off += 1;

  c->damageSuffered = *(uint16_t *)(b + off);
  off += 2;

  c->weaponHit = *(uint16_t *)(b + off);
  off += 2;

  c->totalMightModifier = *(uint16_t *)(b + off);
  off += 2;

  c->totalProtectionModifier = *(uint16_t *)(b + off);
  off += 2;

  c->might = *(uint16_t *)(b + off);
  off += 2;

  c->protection = *(uint16_t *)(b + off);
  off += 2;

  c->nextAnimUpdateCountdown = *(uint16_t *)(b + off);
  off += 2;

  for (int i = 0; i < 11; i++) {
    c->items[i] = *(uint16_t *)(b + off);
    off += 2;
  }
  for (int i = 0; i < 3; i++) {
    c->skillLevels[i] = b[off];
    off += 1;
  }
  for (int i = 0; i < 3; i++) {
    c->skillModifiers[i] = b[off];
    off += 1;
  }
  for (int i = 0; i < 3; i++) {
    c->xpPoints[i] = *(uint16_t *)(b + off);
    off += 2;
  }
  for (int i = 0; i < 5; i++) {
    c->characterUpdateEvents[i] = b[off];
    off += 1;
  }
  for (int i = 0; i < 5; i++) {
    c->characterUpdateDelay[i] = b[off];
    off += 1;
  }
}

static int getSlot(const SAVHandle *handle, SAVSlot *slot) {
  assert(slot);

  slot->header = (SAVHeader *)handle->buffer;
  uint8_t *charBuffer = handle->buffer + CHARACTERS_OFFSET;
  for (int i = 0; i < 4; i++) {
    getCharacter(slot->characters + i, charBuffer);
    charBuffer += SAVCharacterSize;
  }
  const uint8_t *b = (uint8_t *)(handle->buffer + GENERAL_OFFSET);
  int off = 0;

  slot->currentBlock = *(uint16_t *)(b + off);
  off += 2;

  slot->posX = (uint16_t)*(b + off);
  off += 2;

  slot->posY = *(uint16_t *)(b + off);
  off += 2;

  slot->updateFlags = *(uint16_t *)(b + off);
  off += 2;

  slot->scriptDir = b[off];
  off += 1;

  slot->selectedSpell = b[off];
  off += 1;

  off += 2; // skip

  slot->sceneDefaultUpdate = b[off];
  off += 1;

  slot->compassBroken = b[off];
  off += 1;

  slot->drainMagic = b[off];
  off += 1;

  slot->currentDirection = *(uint16_t *)(b + off);
  off += 2;

  slot->compassDirection = *(uint16_t *)(b + off);
  off += 2;

  slot->selectedChar = b[off];
  off += 1;

  off += 1; // skip

  slot->currentLevel = *(b + off);
  off += 1;

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    slot->inventory[i] = *(uint16_t *)(b + off);
    off += 2;
  }

  slot->inventoryCurrentItem = *(uint16_t *)(b + off);
  off += 2;

  slot->itemIndexInHand = *(uint16_t *)(b + off);
  off += 2;

  slot->lastMouseRegion = *(uint16_t *)(b + off);
  off += 2;

  memset(slot->flags, 0, NUM_FLAGS * 2);
  for (int i = 0; i < NUM_FLAGS; i++) {
    slot->flags[i] = *(uint16_t *)(b + off);
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
      if (handle->slot.flags[i] & (1 << k)) {
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
