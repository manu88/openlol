#pragma once
#include "format_tim.h"

typedef struct {
  int16_t procFunc;
  int currentFunc;
  uint16_t procParam;
  TIMHandle *_tim;
} TimInterpreter;

void TimInterpreterInit(TimInterpreter *interp);
int TimInterpreterExec(TimInterpreter *interp, TIMHandle *tim);
