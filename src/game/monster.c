#include "monster.h"
#include "game_ctx.h"
#include <string.h>

void MonsterInit(Monster *monster) {
  memset(monster, 0, sizeof(Monster));
  monster->available = 1;
}

int MonsterGetAvailableSlot(const GameContext *ctx) {
  for (int i = 0; i < MAX_MONSTERS; i++) {
    if (ctx->level->monsters[i].available) {
      return i;
    }
  }
  return -1;
}
