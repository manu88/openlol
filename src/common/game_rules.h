#pragma once
#include "formats/format_sav.h"
#include <stdint.h>

uint16_t GameRuleGetCharacterMight(const SAVCharacter* c);
uint16_t GameRuleGetCharacterProtection(const SAVCharacter* c);
