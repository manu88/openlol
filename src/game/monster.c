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

int MonsterGetNearest(const GameContext *ctx, int charBlock) {
  int minDist = 10000;
  int id = -1;
  Point posChar;
  BlockGetCoordinates(&posChar.x, &posChar.y, charBlock, 0x80, 0x80);

  for (int i = 0; i < MAX_MONSTERS; i++) {
    const Monster *monster = ctx->level->monsters + i;
    Point posMonster;
    BlockGetCoordinates(&posMonster.x, &posMonster.y, monster->block, 0x80,
                        0x80);
    int dist = PointDistance(&posChar, &posMonster);
    if (dist < minDist) {
      minDist = dist;
      id = monster->id;
    }
  }
  return id;
}
