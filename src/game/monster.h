#pragma once
#include <stdint.h>

typedef struct _GameContext GameContext;

typedef struct {
  uint8_t shapeIndex;
  uint8_t maxWidth;
  uint16_t fightingStats[9];
  uint16_t itemsMight[8];
  uint16_t protectionAgainstItems[8];
  uint16_t itemProtection;
  uint16_t hitPoints;
  uint8_t speedTotalWaitTicks;
  uint8_t skillLevel;
  uint16_t flags;
  uint16_t _unknown;
  uint16_t numDistAttacks;
  uint16_t numDistWeapons;
  uint16_t distWeapons[3];
  uint8_t attackSkillChance;
  uint8_t attackSkillType;
  uint8_t defenseSkillChance;
  uint8_t defenseSkillType;
  uint8_t sounds[3];
} MonsterProperties;

typedef struct {
  uint8_t available;
  uint8_t destDirection;
  int8_t shiftStep;
  uint16_t destX;
  uint16_t destY;

  int8_t hitOffsX;
  int8_t hitOffsY;
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
  MonsterProperties *properties;
  uint8_t numDistAttacks;
  uint8_t curDistWeapon;
  int8_t distAttackTick;
  uint16_t assignedItems;
  uint8_t equipmentShapes[4];
} Monster;

void MonsterInit(Monster *monster);
int MonsterGetAvailableSlot(const GameContext *ctx);
