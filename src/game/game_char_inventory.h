#pragma once
#include "game_ctx.h"

typedef enum {
  CharItemIndex_Hand = 0,
  CharItemIndex_Shield = 1,
  CharItemIndex_Hand2 = 2,
  CharItemIndex_Shield2 = 3,
  CharItemIndex_Helm = 4,
  CharItemIndex_Armor = 5,
  CharItemIndex_Necklace = 6,
  CharItemIndex_Braces = 7,
  CharItemIndex_Shoes = 8,
  CharItemIndex_RingLeft = 9,
  CharItemIndex_RingRight = 10,
} CharItemIndex;

void renderInventoryStrip(GameContext *gameCtx);
void renderCharInventory(GameContext *gameCtx);
