#pragma once
#include "format_tim.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
typedef struct {
  TIMHandle *tim;
  TIMInterpreter _interpreter;

  SDL_Window *window;
} TIMAnimator;

void TIMAnimatorInit(TIMAnimator *animator);
void TIMAnimatorRelease(TIMAnimator *animator);

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim);
