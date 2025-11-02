#include "game_rules.h"
#include "formats/format_sav.h"
#include <stdio.h>

static const uint16_t classDefaultModifiers_Man[9] = {
    0x0100, 0x0100, 0x0100, 0x0032, 0x0100, 0x0100, 0x0100, 0x0100, 0x0004};

static const uint16_t classDefaultModifiers_Woman[9] = {
    0x0100, 0x0100, 0x0100, 0x0032, 0x0100, 0x0100, 0x0100, 0x0100, 0x0004};

static const uint16_t classDefaultModifiers_Uline[9] = {
    0x0180, 0x0080, 0x00C0, 0x0032, 0x0200, 0x00A6, 0x0100, 0x0140, 0x0006};

static const uint16_t classDefaultModifiers_Dracoid[9] = {
    0x00C0, 0x00C0, 0x0140, 0x0032, 0x0100, 0x0180, 0x0180, 0x0100, 0x0004};

static const uint16_t *classDefaultModifiers[] = {
    classDefaultModifiers_Man, classDefaultModifiers_Woman,
    classDefaultModifiers_Uline, classDefaultModifiers_Man,
    classDefaultModifiers_Dracoid};

static const uint32_t experienceRequirements[11] = {
    0x00000000, 0x000001F4, 0x000005DC, 0x00001388, 0x000061A8, 0x0000C350,
    0x00013880, 0x0001D4C0, 0x0002BF20, 0x00041EB0, 0x7FFFFFFF};

uint16_t GameRuleGetCharacterMight(const SAVCharacter *c) {
  int might = 0;
  for (int i = 0; i < 8; i++) {
    might += c->itemsMight[i];
  }
  if (might) {
    might += c->might;
  } else {
    might += classDefaultModifiers[c->raceClassSex][8];
  }

  might = (might * classDefaultModifiers[c->raceClassSex][1]) >> 8;
  might = (might * c->totalMightModifier) >> 8;
  return might;
}

uint16_t GameRuleGetCharacterProtection(const SAVCharacter *c) {
  uint16_t prot = c->itemProtection + c->protection;
  prot = (prot * classDefaultModifiers[c->raceClassSex][2]) >> 8;
  prot = (prot * c->totalProtectionModifier) >> 8;
  return prot;
}

uint16_t GameRuleGetCharacterSkillFight(const SAVCharacter *c) {
  return c->skillLevels[SkillIndex_Fighter] +
         c->skillModifiers[SkillIndex_Fighter];
}

uint16_t GameRuleGetCharacterSkillRogue(const SAVCharacter *c) {
  return c->skillLevels[SkillIndex_Rogue] + c->skillModifiers[SkillIndex_Rogue];
}

uint16_t GameRuleGetCharacterSkillMage(const SAVCharacter *c) {
  return c->skillLevels[SkillIndex_Mage] + c->skillModifiers[SkillIndex_Mage];
}

void GameRuleGetCharacterExpPoints(const SAVCharacter *c, SkillIndex skill,
                                   int32_t *points, int32_t *requirements) {
  int currentLevel = c->skillLevels[skill];
  *points = c->xpPoints[skill] - experienceRequirements[currentLevel - 1];

  *requirements = experienceRequirements[currentLevel] -
                  experienceRequirements[currentLevel - 1];
  if (*points < 0) {
    *points = 0;
  }
  if (*points > *requirements) {
    *points = *requirements;
  }
}
