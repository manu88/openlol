#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t flags;
  char name[11];
  uint8_t raceClassSex;
  int16_t id; // negative number means this is the hero
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
  uint16_t uu;
  uint16_t items[11];
  // uint16_t skillLevels[11];
  //  uint16_t skillModifiers[11];
  //  uint16_t xpPoints[11];
  //  uint16_t characterUpdateEvents[11];
  //  uint16_t characterUpdateDelay[11];

} SAVCharacter;

#define SAVCharacterSize 0X82

#define INVENTORY_SIZE 48

#define NUM_FLAGS 40

typedef struct {
  uint16_t currentBlock;
  uint16_t posX;
  uint16_t posY;
  uint16_t updateFlags;
  uint8_t scriptDir;
  uint8_t selectedSpell;

  uint8_t sceneDefaultUpdate;
  uint8_t compassBroken;

  uint8_t drainMagic;
  uint8_t currentDirection;

  uint16_t compassDirection;
  uint8_t selectedChar;

  uint8_t currentLevel;

  uint16_t inventory[INVENTORY_SIZE];
  uint16_t inventoryCurrentItem;
  uint16_t itemIndexInHand;
  uint16_t lastMouseRegion;

  uint16_t flags[NUM_FLAGS]; // this gets transformed into gameFlags using
                             // SAVHandleGetGameFlags
} SAVGeneral;

#define NUM_GLOBAL_SCRIPT_VARS2 8

typedef struct {
  uint8_t brightness;
  uint8_t lampOilStatus;
  uint8_t lampEffect;
  uint8_t _;
  uint16_t credits;

  uint16_t globalScriptVars[NUM_GLOBAL_SCRIPT_VARS2];
} SAVGeneral2;

typedef struct {
  char name[46];
} SAVHeader;

#define MAX_ITEM_ID 0X89

typedef struct __attribute__((__packed__)) {
  uint16_t nextAssignedObject;
  uint16_t nextDrawObject;
  uint8_t flyingHeight;
  uint16_t block;
  uint16_t x;
  uint16_t y;
  uint8_t level;
  uint16_t itemPropertyIndex;
  uint16_t shpCurFrame_flg;
} GameObject;

static_assert(sizeof(GameObject) == 16, "");

#define NUM_CHARACTERS 3

typedef struct {
  SAVHeader *header;
  SAVCharacter *characters[NUM_CHARACTERS];

  SAVGeneral _general;
  SAVGeneral *general;
  SAVGeneral2 *general2;

  GameObject *gameObjects;
} SAVSlot;

typedef struct {
  uint8_t *buffer;
  size_t bufferSize;

  SAVSlot slot;
} SAVHandle;

int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize);
void SAVHandleGetGameFlags(const SAVHandle *handle, uint8_t *gameFlags,
                           size_t numGameFlags);
