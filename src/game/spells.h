#pragma once
#include <stdint.h>

#define SPELL_PROPERTIES_COUNT 17
typedef struct {
  uint16_t spellNameCode;
  uint16_t mpRequired[4];
  uint16_t field_a;
  uint16_t field_c;
  uint16_t hpRequired[4];
  uint16_t field_16;
  uint16_t field_18;
  uint16_t flags;
} SpellProperties;

const SpellProperties *SpellPropertiesGet(void);
