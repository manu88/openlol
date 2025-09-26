#pragma once
#include "format_tim.h"
#include <stddef.h>
#include <stdint.h>

typedef struct _TIMInterpreter TIMInterpreter;

typedef struct {
  void (*TIMInterpreterCallbacks_WSAInit)(TIMInterpreter *interp,
                                          const char *wsaFile, int x, int y,
                                          int offscreen, int flags);
  void (*TIMInterpreterCallbacks_WSARelease)(TIMInterpreter *interp, int index);
  void (*TIMInterpreterCallbacks_WSADisplayFrame)(TIMInterpreter *interp,
                                                  int frameIndex, int frame);
  void (*TIMInterpreterCallbacks_PlayDialogue)(TIMInterpreter *interp,
                                               uint16_t strId);
  void (*TIMInterpreterCallbacks_ShowButtons)(TIMInterpreter *interp,
                                              uint16_t functionId,
                                              const uint16_t buttonStrIds[3]);
  void (*TIMInterpreterCallbacks_InitSceneDialog)(TIMInterpreter *interp,
                                                  int controlMode);
} TIMInterpreter2Callbacks;

typedef struct _TIMInterpreter {
  TIMInterpreter2Callbacks callbacks;
  void *callbackCtx;

  TIMHandle *_tim;
  size_t pos;

  uint8_t dontLoop; // just list instructions

  int loopStartPos;
  int restartLoop;
  int inLoop;

  int buttonState[3];

} TIMInterpreter;

void TIMInterpreterInit(TIMInterpreter *interp);
void TIMInterpreterRelease(TIMInterpreter *interp);

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim,
                         uint32_t timeMs);
int TIMInterpreterIsRunning(const TIMInterpreter *interp);
void TIMInterpreterUpdate(TIMInterpreter *interp, uint32_t timeMs);
void TIMInterpreterButtonClicked(TIMInterpreter *interp, int buttonIndex);
