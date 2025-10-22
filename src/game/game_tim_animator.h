#pragma once

#include "animator.h"
#include "formats/format_tim.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
#include <stdint.h>

#define NUM_TIM_ANIMATIONS 4

typedef struct {

  TIMInterpreter timInterpreter;
  uint16_t currentTimScriptId;

  TIMHandle tim[NUM_TIM_ANIMATIONS];
  Animator *animator;
} GameTimInterpreter;

typedef struct _GameContext GameContext;

void GameTimInterpreterInit(GameTimInterpreter *animator, Animator *animator_);
void GameTimInterpreterRelease(GameTimInterpreter *animator);
void GameTimInterpreterLoadTim(GameTimInterpreter *animator, uint16_t scriptId,
                               const char *file);
void GameTimInterpreterRunTim(GameTimInterpreter *animator, uint16_t scriptId);
void GameTimInterpreterReleaseTim(GameTimInterpreter *animator,
                                  uint16_t scriptId);

int GameTimInterpreterRender(GameTimInterpreter *animator);
