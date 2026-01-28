#include "format_sav.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CHARACTERS_OFFSET 0X4C
#define GENERAL_OFFSET 0X254
#define ALL_OBJECTS_OFFSET 0X487
#define GLOBAL_SCRIPT_DATA 0X398

typedef struct _LoadSaveOps {
  int (*LoadSave)(struct _LoadSaveOps *ops, void *field, size_t size);
  int (*Skip)(struct _LoadSaveOps *ops, size_t size);
  void (*Finalize)(struct _LoadSaveOps *ops);

  size_t pos;
  size_t bufferSize;
  void *ctx;
} LoadSaveOps;

#pragma mark Load SAV file

static int loadField(LoadSaveOps *ops, void *field, size_t size) {
  if (ops->bufferSize) {
    assert(ops->pos < ops->bufferSize);
  }
  uint8_t *buffer = (uint8_t *)ops->ctx + ops->pos;
  memcpy(field, buffer, size);
  return size;
}

static int loadSkip(LoadSaveOps *ops, size_t size) {
  if (ops->bufferSize) {
    assert(ops->pos < ops->bufferSize);
  }
  return size;
}

static LoadSaveOps loadOps = {
    .LoadSave = loadField,
    .Skip = loadSkip,
    .Finalize = NULL,
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

static void saveFinalize(LoadSaveOps *ops) {
  FILE *outFile = ops->ctx;
  ftruncate(fileno(outFile), ops->pos);
}

static LoadSaveOps saveOps = {
    .LoadSave = saveField,
    .Skip = saveSkip,
    .Finalize = saveFinalize,
};

#define FIELD(field) op->pos += op->LoadSave(op, &field, sizeof(field))
#define SKIP(size) op->pos += op->Skip(op, size)

#define TEMP_DATA_SIZE 2500

static void loadTempDataLevel(SAVSlot *slot, int levelId, LoadSaveOps *op,
                              void *ctx) {

  TempLevelData *tempData = slot->tempLevelData + levelId;
  SKIP(4);
  FIELD(tempData->origCmp);

  FIELD(tempData->monsterDifficulty);
  for (int monsterIndex = 0; monsterIndex < MAX_MONSTERS; monsterIndex++) {
    Monster *m = tempData->monsters + monsterIndex;
    size_t startMonster = op->pos;
    FIELD(m->nextAssignedObject);
    FIELD(m->nextDrawObject);
    FIELD(m->flyingHeight);
    FIELD(m->block);
    FIELD(m->x);
    FIELD(m->y);
    FIELD(m->shiftStep);
    FIELD(m->destX);
    FIELD(m->destY);
    FIELD(m->destDirection);
    FIELD(m->hitOffsX);
    FIELD(m->hitOffsY);
    FIELD(m->currentSubFrame);
    FIELD(m->mode);
    FIELD(m->fightCurTick);
    FIELD(m->id);
    FIELD(m->direction);
    FIELD(m->facing);
    FIELD(m->flags);
    FIELD(m->damageReceived);
    FIELD(m->hitPoints);
    FIELD(m->speedTick);
    FIELD(m->type);
    SKIP(4);
    FIELD(m->numDistAttacks);
    FIELD(m->curDistWeapon);
    FIELD(m->distAttackTick);
    FIELD(m->assignedItems);
    FIELD(m->equipmentShapes);
    assert(op->pos - startMonster == 46);
  }
  for (int flyingObjIndex = 0; flyingObjIndex < NUM_FLYING_OBJECTS;
       flyingObjIndex++) {
    FlyingObject *obj = tempData->flyingObjects + flyingObjIndex;
    FIELD(obj->enable);
    FIELD(obj->objectType);
    FIELD(obj->attackerId);
    FIELD(obj->item);
    FIELD(obj->x);
    FIELD(obj->y);
    FIELD(obj->flyingHeight);
    FIELD(obj->direction);
    FIELD(obj->distance);
    FIELD(obj->fieldD);
    FIELD(obj->c);
    FIELD(obj->flags);
    FIELD(obj->wallFlags);
  }
}

int SAVSlotHasTempLevelData(const SAVSlot *slot, int levelId) {
  return slot->numTempDataFlags & (1 << levelId);
}

static void loadTempData(SAVSlot *slot, LoadSaveOps *op, void *ctx) {
  for (int levelId = 0; levelId < NUM_LEVELS; levelId++) {
    if (!SAVSlotHasTempLevelData(slot, levelId)) {
      SKIP(TEMP_DATA_SIZE);
      continue;
    }
    size_t next = op->pos + TEMP_DATA_SIZE;
    loadTempDataLevel(slot, levelId, op, ctx);

    size_t off = next - op->pos;
    SKIP(off);
  }
}

static void loadSaveHeader(SAVHeader *header, LoadSaveOps *op, void *ctx) {
  FIELD(header->name);
  SKIP(14);
  FIELD(header->type);
  SKIP(4);
  FIELD(header->version);
}

/*
This function defines the SAV format layout for both loading and saving files.
*/
static void loadSaveSlot(SAVSlot *slot, LoadSaveOps *op, void *ctx) {
  loadSaveHeader(&slot->header, op, ctx);

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

  assert(op->pos == GENERAL_OFFSET);
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

  assert(op->pos == GLOBAL_SCRIPT_DATA);
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

  SKIP(6);

  assert(op->pos == ALL_OBJECTS_OFFSET);
  for (int i = 0; i < MAX_IN_GAME_ITEMS; i++) {
    FIELD(slot->gameObjects[i].nextAssignedObject);
    FIELD(slot->gameObjects[i].nextDrawObject);
    FIELD(slot->gameObjects[i].flyingHeight);
    FIELD(slot->gameObjects[i].block);
    FIELD(slot->gameObjects[i].x);
    FIELD(slot->gameObjects[i].y);
    FIELD(slot->gameObjects[i].level);
    FIELD(slot->gameObjects[i].itemPropertyIndex);
    FIELD(slot->gameObjects[i].shpCurFrame_flg);
  }

  loadTempData(slot, op, ctx);

  if (op->Finalize) {
    op->Finalize(op);
  }
}

int SAVHeaderFromBuffer(SAVHeader *header, uint8_t *buffer, size_t bufferSize) {
  memset(header, 0, sizeof(SAVHeader));
  loadOps.bufferSize = bufferSize;
  loadOps.pos = 0;
  loadOps.ctx = buffer;
  loadSaveHeader(header, &loadOps, buffer);
  return 1;
}

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize) {
  memset(&handle->slot, 0, sizeof(SAVSlot));
  loadOps.bufferSize = bufferSize;
  loadOps.pos = 0;
  loadOps.ctx = buffer;
  loadSaveSlot(&handle->slot, &loadOps, buffer);
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
  saveOps.pos = 0;
  saveOps.ctx = outFile;
  loadSaveSlot((SAVSlot *)&handle->slot, &saveOps, outFile);
  fclose(outFile);
  return 1;
}
