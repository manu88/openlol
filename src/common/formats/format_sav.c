#include "format_sav.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHARACTERS_OFFSET 0X4C
#define GENERAL_OFFSET 0X254
#define ALL_OBJECTS_OFFSET 0X487

typedef struct _LoadSaveOps {
  int (*LoadSave)(struct _LoadSaveOps *ops, void *field, size_t size);
  int (*Skip)(struct _LoadSaveOps *ops, size_t size);

  size_t globalOffset;
  void *ctx;
} LoadSaveOps;

#pragma mark Load SAV file

static int loadField(LoadSaveOps *ops, void *field, size_t size) {
  uint8_t *buffer = (uint8_t *)ops->ctx + ops->globalOffset;
  memcpy(field, buffer, size);
  return size;
}

static int loadSkip(LoadSaveOps *ops, size_t size) { return size; }

static LoadSaveOps loadOps = {
    .LoadSave = loadField,
    .Skip = loadSkip,
};

#pragma mark Save SAV file

static int saveField(LoadSaveOps *ops, void *field, size_t size) {
  FILE *outFile = ops->ctx;
  fwrite(field, size, 1, outFile);
  return size;
}

static int saveSkip(LoadSaveOps *ops, size_t size) {
  FILE *outFile = ops->ctx;
  fseek(outFile, size, SEEK_CUR);
  return size;
}

static LoadSaveOps saveOps = {
    .LoadSave = saveField,
    .Skip = saveSkip,
};

#define FIELD(field) op->globalOffset += op->LoadSave(op, &field, sizeof(field))
#define SKIP(size) op->globalOffset += op->Skip(op, size)

/*
This function defines the SAV format layout for both loading and saving files.
*/
static void loadSaveChar(SAVSlot *slot, LoadSaveOps *op, void *ctx) {
  op->globalOffset = 0;
  op->ctx = ctx;
  // Header
  FIELD(slot->header->name);
  SKIP(14);
  FIELD(slot->header->type);
  SKIP(4);
  FIELD(slot->header->version);

  // Characters
  for (int i = 0; i <= NUM_CHARACTERS; i++) {
    FIELD(slot->characters[i].flags);
    FIELD(slot->characters[i].name);
    FIELD(slot->characters[i].raceClassSex);
    FIELD(slot->characters[i].id);
    FIELD(slot->characters[i].currentFaceFrame);
    FIELD(slot->characters[i].tempFaceFrame);
    FIELD(slot->characters[i].screamSfx);
    SKIP(4);
    FIELD(slot->characters[i].itemsMight);
    FIELD(slot->characters[i].itemsProtection);
    FIELD(slot->characters[i].itemProtection);
    FIELD(slot->characters[i].hitPointsCur);
    FIELD(slot->characters[i].hitPointsMax);
    FIELD(slot->characters[i].magicPointsCur);
    FIELD(slot->characters[i].magicPointsMax);
    FIELD(slot->characters[i].unknown_41);
    FIELD(slot->characters[i].damageSuffered);
    FIELD(slot->characters[i].weaponHit);
    FIELD(slot->characters[i].totalMightModifier);
    FIELD(slot->characters[i].totalProtectionModifier);
    FIELD(slot->characters[i].might);
    FIELD(slot->characters[i].protection);
    FIELD(slot->characters[i].nextAnimUpdateCountdown);
    FIELD(slot->characters[i].items);
    FIELD(slot->characters[i].skillLevels);
    FIELD(slot->characters[i].skillModifiers);
    FIELD(slot->characters[i].xpPoints);
    FIELD(slot->characters[i].characterUpdateEvents);
    FIELD(slot->characters[i].characterUpdateDelay);
  }

  assert(op->globalOffset == GENERAL_OFFSET);
  FIELD(slot->currentBlock);
  FIELD(slot->posX);
  FIELD(slot->posY);
  FIELD(slot->updateFlags);
  FIELD(slot->scriptDir);
  FIELD(slot->selectedSpell);
  SKIP(2);
  FIELD(slot->sceneDefaultUpdate);
  FIELD(slot->compassBroken);
  FIELD(slot->drainMagic);
  FIELD(slot->currentDirection);
  SKIP(1);
  FIELD(slot->compassDirection);
  FIELD(slot->selectedChar);
  SKIP(1);
  FIELD(slot->currentLevel);

  FIELD(slot->inventory);

  FIELD(slot->inventoryCurrentItem);
  FIELD(slot->itemIndexInHand);
  FIELD(slot->lastMouseRegion);

  FIELD(slot->flags);

  SKIP(120);

  FIELD(slot->globalScriptVars);
  SKIP(152);

  FIELD(slot->brightness);
  FIELD(slot->lampOilStatus);
  FIELD(slot->lampEffect);
  SKIP(1);
  FIELD(slot->credits);
  FIELD(slot->globalScriptVars2);
  FIELD(slot->spells);
  FIELD(slot->numTempDataFlags);

  slot->gameObjects = slot->gameObjs;
  SKIP(6);

  assert(op->globalOffset == ALL_OBJECTS_OFFSET);
  for (int i = 0; i < MAX_IN_GAME_ITEMS; i++) {
    FIELD(slot->gameObjs[i].nextAssignedObject);
    FIELD(slot->gameObjs[i].nextDrawObject);
    FIELD(slot->gameObjs[i].flyingHeight);
    FIELD(slot->gameObjs[i].block);
    FIELD(slot->gameObjs[i].x);
    FIELD(slot->gameObjs[i].y);
    FIELD(slot->gameObjs[i].level);
    FIELD(slot->gameObjs[i].itemPropertyIndex);
    FIELD(slot->gameObjs[i].shpCurFrame_flg);
  }
}

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize) {
  handle->slot.header = &handle->slot._header;
  loadSaveChar(&handle->slot, &loadOps, buffer);
  return 1;
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

int SAVHandleSaveTo(const SAVHandle *handle, const char *filepath) {

  FILE *outFile = fopen(filepath, "wb");
  if (outFile == NULL) {
    return 0;
  }
  loadSaveChar((SAVSlot *)&handle->slot, &saveOps, outFile);
  fclose(outFile);

  return 1;
}
