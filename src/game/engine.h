#pragma once
#include "formats/format_inf.h"
#include "formats/format_sav.h"
#include "script.h"

#define NUM_GAME_FLAGS 100

typedef struct {
  uint16_t globalScriptVars[NUM_GLOBAL_SCRIPT_VARS];
  uint8_t gameFlags[NUM_GAME_FLAGS];

  INFScript itemScript;
  // INFScript script;
  // uint16_t nextFunc;
  // EMCInterpreter interp;
  // EMCState interpState;

} GameEngine;

int GameEngineInit(GameEngine *engine);
void GameEngineRelease(GameEngine *engine);

uint16_t GameEngineGetGameFlag(const GameEngine *engine, uint16_t flag);
void GameEngineSetGameFlag(GameEngine *engine, uint16_t flag);
void GameEngineResetGameFlag(GameEngine *engine, uint16_t flag);
