#pragma once

#include "SDL_render.h"
#include "animator.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
#include <stdint.h>

#define NUM_TIM_ANIMATIONS 4

typedef struct {

  TIMInterpreter timInterpreter;
  uint16_t currentTimScriptId;

  TIMHandle tim[NUM_TIM_ANIMATIONS];
  Animator *animator;
} GameTimAnimator;

typedef struct _GameContext GameContext;

void GameTimAnimatorInit(GameTimAnimator *animator, Animator *animator_);
void GameTimAnimatorRelease(GameTimAnimator *animator);
void GameTimAnimatorLoadTim(GameTimAnimator *animator, uint16_t scriptId,
                            const char *file);
void GameTimAnimatorRunTim(GameTimAnimator *animator, uint16_t scriptId);
void GameTimAnimatorReleaseTim(GameTimAnimator *animator, uint16_t scriptId);

int GameTimAnimatorRender(GameTimAnimator *animator);

void GameTimAnimatorWSAInit(GameTimAnimator *animator, uint16_t index,
                            const char *wsaFile, int x, int y, int offscreen,
                            int flags);
