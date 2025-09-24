#pragma once
#include "format_tim.h"

typedef struct {
  int16_t procFunc;
  int currentFunc;
  uint16_t procParam;
  TIMHandle *_tim;

  int looped;
} TIMInterpreter;

void TIMInterpreterInit(TIMInterpreter *interp);
void TIMInterpreterRelease(TIMInterpreter *interp);
int TIMInterpreterExec(TIMInterpreter *interp, TIMHandle *tim);
