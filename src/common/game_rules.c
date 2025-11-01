#include "game_rules.h"

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
