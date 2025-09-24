#pragma once
#include "format_tim.h"

typedef struct {
  int16_t procFunc;
  int currentFunc;
  uint16_t procParam;
  TIMHandle *_tim;

  int looped;
  int running;
  TimFunction *cur;
} TIMInterpreter;

void TIMInterpreterInit(TIMInterpreter *interp);
void TIMInterpreterRelease(TIMInterpreter *interp);

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim);
int TIMInterpreterIsRunning(const TIMInterpreter *interp);
void TIMInterpreterUpdate(TIMInterpreter *interp);
