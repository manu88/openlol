#include "engine.h"

int GameEngineInit(GameEngine *engine) { return 1; }

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
