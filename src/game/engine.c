#include "engine.h"
#include "game_envir.h"
#include <string.h>

int GameEngineInit(GameEngine *engine) {
  memset(engine, 0, sizeof(GameEngine));
  GameFile f = {0};
  assert(GameEnvironmentGetStartupFile(&f, "ITEM.INF"));
  if (INFScriptFromBuffer(&engine->itemScript, f.buffer, f.bufferSize) == 0) {
    printf("unable to get ITEMS.INF\n");
    assert(0);
  }

  return 1;
}

void GameEngineRelease(GameEngine *engine) {}

uint16_t GameEngineGetGameFlag(const GameEngine *engine, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(engine->gameFlags));
  return ((engine->gameFlags[flag >> 3] >> (flag & 7)) & 1);
}

void GameEngineSetGameFlag(GameEngine *engine, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(engine->gameFlags));
  engine->gameFlags[flag >> 3] |= (1 << (flag & 7));
}

void GameEngineResetGameFlag(GameEngine *engine, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(engine->gameFlags));
  engine->gameFlags[flag >> 3] &= ~(1 << (flag & 7));
}
