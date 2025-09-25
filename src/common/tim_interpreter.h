#pragma once
#include "format_tim.h"
#include <stdint.h>

typedef struct _TIMInterpreter TIMInterpreter;

typedef struct {
  void (*TIMInterpreterCallbacks_WSAInit)(TIMInterpreter *interp,
                                          const char *wsaFile);
  void (*TIMInterpreterCallbacks_WSADisplayFrame)(TIMInterpreter *interp,
                                                  int frameIndex, int frame);
  void (*TIMInterpreterCallbacks_PlayDialogue)(TIMInterpreter *interp,
                                               uint16_t strId);
  void (*TIMInterpreterCallbacks_ShowButtons)(TIMInterpreter *interp,
                                              uint16_t buttonStrIds[3],
                                              int numButtons);
} TIMInterpreterCallbacks;

typedef struct _TIMInterpreter {
  TIMInterpreterCallbacks callbacks;
  void *callbackCtx;

  int16_t procFunc;
  int currentFunc;
  int nextFunc;
  uint16_t procParam;
  TIMHandle *_tim;

  int state;
  int looped;
  int running;
  TimFunction *cur;
} TIMInterpreter;

void TIMInterpreterInit(TIMInterpreter *interp);
void TIMInterpreterRelease(TIMInterpreter *interp);

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim);
int TIMInterpreterIsRunning(const TIMInterpreter *interp);
void TIMInterpreterUpdate(TIMInterpreter *interp);
