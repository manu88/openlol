#pragma once
#include "formats/format_sav.h"
#include <stdint.h>

uint16_t GameRuleGetCharacterMight(const SAVCharacter *c);
uint16_t GameRuleGetCharacterProtection(const SAVCharacter *c);

uint16_t GameRuleGetCharacterSkillFight(const SAVCharacter *c);
uint16_t GameRuleGetCharacterSkillRogue(const SAVCharacter *c);
uint16_t GameRuleGetCharacterSkillMage(const SAVCharacter *c);

void GameRuleGetCharacterExpPoints(const SAVCharacter *c, SkillIndex skill,
                                   int32_t *points, int32_t *requirements);
