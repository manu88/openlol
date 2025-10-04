#pragma once

#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "tim_interpreter.h"
#include <stdint.h>

#define NUM_TIM_ANIMATIONS 4

typedef struct {
  TIMInterpreter timInterpreter;
  uint16_t currentTimScriptId;

  WSAHandle wsa;

  TIMHandle tim[NUM_TIM_ANIMATIONS];
} GameTimAnimator;

void GameTimAnimatorInit(GameTimAnimator *animator);

void GameTimAnimatorLoadTim(GameTimAnimator *animator, uint16_t scriptId,
                            const char *file);
void GameTimAnimatorRunTim(GameTimAnimator *animator, uint16_t scriptId);
void GameTimAnimatorReleaseTim(GameTimAnimator *animator, uint16_t scriptId);

void GameTimAnimatorRender(GameTimAnimator *animator);
