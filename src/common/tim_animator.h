#pragma once
#include "format_tim.h"
#include "tim_interpreter.h"
typedef struct {
  TIMHandle *tim;
  TIMInterpreter _interpreter;
} TIMAnimator;

void TIMAnimatorInit(TIMAnimator *animator);
void TIMAnimatorRelease(TIMAnimator *animator);

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim);
