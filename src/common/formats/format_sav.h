#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum {
  SkillIndex_Fighter = 0,
  SkillIndex_Rogue = 1,
  SkillIndex_Mage = 2,
} SkillIndex;

typedef struct __attribute__((__packed__)) {
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

  uint16_t items[11];
  uint8_t skillLevels[3];    // SkillIndex 0: fighter, 1: rogue, 2: mage
  uint8_t skillModifiers[3]; // SkillIndex 0: fighter, 1: rogue, 2: mage
  uint32_t xpPoints[3];      // SkillIndex 0: fighter, 1: rogue, 2: mage
  uint8_t characterUpdateEvents[5];
  uint8_t characterUpdateDelay[5];

  uint32_t _skip; // not an actual field, used to skip bytes when loading/saving
                  // files
} SAVCharacter;

#define SAVCharacterSize 0X82

#define INVENTORY_SIZE 48

#define NUM_FLAGS 40

#define NUM_GLOBAL_SCRIPT_VARS 24
#define NUM_GLOBAL_SCRIPT_VARS2 8

#define NUM_LEVELS 29
#define NUM_FLYING_OBJECTS 8

#define SAV_NAME_FILE_MAX_SIZE 46

#define MAX_IN_GAME_ITEMS 400
#define SAV_HEADER_SIZE 76
#define MAX_MONSTERS 30

typedef struct {
  char name[SAV_NAME_FILE_MAX_SIZE];
  char type[4];
  char version[8];
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

typedef struct {
  uint8_t available; // This field is not part of the original code i.e not
                     // serialized
  uint16_t nextAssignedObject;
  uint16_t nextDrawObject;
  uint8_t flyingHeight;
  uint16_t block;
  uint16_t x;
  uint16_t y;
  int8_t shiftStep;
  uint16_t destX;
  uint16_t destY;
  uint8_t destDirection;
  uint8_t hitOffsX;
  uint8_t hitOffsY;
  uint8_t currentSubFrame;
  uint8_t mode;
  int8_t fightCurTick;
  uint8_t id;
  uint8_t direction;
  uint8_t facing;
  uint16_t flags;
  uint16_t damageReceived;
  int16_t hitPoints;
  uint8_t speedTick;
  uint8_t type;
  uint8_t numDistAttacks;
  uint8_t curDistWeapon;
  int8_t distAttackTick;
  uint16_t assignedItems;
  uint32_t equipmentShapes;
} Monster;

typedef struct {
  uint8_t enable;
  uint8_t objectType;
  uint16_t attackerId;
  int16_t item;
  uint16_t x;
  uint16_t y;
  uint8_t flyingHeight;
  uint8_t direction;
  uint8_t distance;
  int8_t fieldD;
  uint8_t c;
  uint8_t flags;
  uint8_t wallFlags;
} FlyingObject;

typedef struct {
  uint16_t monsterDifficulty;
  Monster monsters[MAX_MONSTERS];
  FlyingObject flyingObjects[NUM_FLYING_OBJECTS];

  uint16_t origCmp;
} TempLevelData;

#define NUM_CHARACTERS 3

typedef struct {
  SAVHeader header;
  SAVCharacter characters[NUM_CHARACTERS];

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

  uint16_t globalScriptVars[NUM_GLOBAL_SCRIPT_VARS];

  uint8_t brightness;
  uint8_t lampOilStatus;
  uint8_t lampEffect;

  uint16_t credits;

  uint16_t globalScriptVars2[NUM_GLOBAL_SCRIPT_VARS2];

  uint8_t spells[7];

  uint32_t numTempDataFlags;

  GameObject gameObjects[MAX_IN_GAME_ITEMS];

  TempLevelData tempLevelData[NUM_LEVELS];
} SAVSlot;

typedef struct {
  uint8_t *buffer;
  size_t bufferSize;

  SAVSlot slot;
} SAVHandle;

int SAVHeaderFromBuffer(SAVHeader *header, uint8_t *buffer, size_t bufferSize);
int SAVHandleFromBuffer(SAVHandle *handle, uint8_t *buffer, size_t bufferSize);

void SAVHandleGetGameFlags(const SAVHandle *handle, uint8_t *gameFlags,
                           size_t numGameFlags);
int SAVHandleSaveTo(const SAVHandle *handle, const char *filepath);

int SAVSlotHasTempLevelData(const SAVSlot *slot, int levelId);
