#pragma once
#include "formats/format_sav.h"
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

void MonsterInit(Monster *monster);
int MonsterGetAvailableSlot(const GameContext *ctx);

// returns -1 if none
int MonsterGetNearest(const GameContext *ctx, int charBlock);
