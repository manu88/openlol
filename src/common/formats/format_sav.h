#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t flags;
  char name[11];
  uint8_t raceClassSex;
  uint16_t id;
  uint8_t currentFaceFrame;
  uint8_t tempFaceFrame;
  uint8_t screamSfx;
  uint16_t itemsMight[8];
  uint16_t itemsProtection[8];
  uint16_t itemProtection;

  int16_t hitPointsCur;
  int16_t hitPointsMax;

  int16_t magicPointsCur;
  int16_t magicPointsMax;
  uint8_t unknown_41;

  uint16_t damageSuffered;
  uint16_t weaponHit;

  uint16_t totalMightModifier;
  uint16_t totalProtectionModifier;
  uint16_t might;
  uint16_t protection;
  int16_t nextAnimUpdateCountdown;

  uint16_t items[11];
  // uint16_t skillLevels[11];
  //  uint16_t skillModifiers[11];
  //  uint16_t xpPoints[11];
  //  uint16_t characterUpdateEvents[11];
  //  uint16_t characterUpdateDelay[11];

} SAVCharacter;

#define SAVCharacterSize 0X82

typedef struct {
  uint16_t currentBlock;
  uint16_t posX;
  uint16_t posY;
  uint16_t updateFlags;
  uint8_t scriptDir;
  uint8_t selectedSpell;

  uint16_t unused_0;

  uint8_t sceneDefaultUpdate;
  uint8_t compassBroken;

  uint8_t drainMagic;
  uint8_t currentDirection;

  uint8_t unused_1;
  uint16_t compassDirection;
  uint8_t selectedChar;

  uint8_t unused_2;

  uint8_t currentLevel;
} SAVGeneral;

typedef struct {
  char name[46];
} SAVHeader;

#define INVENTORY_SIZE 48
#define MAX_ITEM_ID 0X89

typedef struct {
  SAVHeader *header;
  SAVCharacter *characters[4];

  SAVGeneral *general;

  int16_t *inventory; // size is INVENTORY_SIZE
} SAVSlot;

typedef struct {
  uint8_t *buffer;
  size_t bufferSize;

  SAVSlot slot;
} SAVHandle;

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize);
