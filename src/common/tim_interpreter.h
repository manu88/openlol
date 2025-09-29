#pragma once
#include "formats/format_tim.h"
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
                                               uint16_t strId, int argc,
                                               const uint16_t *argv);
  void (*TIMInterpreterCallbacks_ShowButtons)(TIMInterpreter *interp,
                                              uint16_t functionId,
                                              const uint16_t buttonStrIds[3]);
  void (*TIMInterpreterCallbacks_InitSceneDialog)(TIMInterpreter *interp,
                                                  int controlMode);
} TIMInterpreterCallbacks;

typedef struct _TIMInterpreter {
  TIMInterpreterCallbacks callbacks;
  void *callbackCtx;

  TIMHandle *_tim;
  size_t pos;

  uint8_t dontLoop; // just list instructions

  int loopStartPos;
  int restartLoop;
  int inLoop;

  int buttonState[3];

  int currentInstructionDuration;

} TIMInterpreter;

void TIMInterpreterInit(TIMInterpreter *interp);
void TIMInterpreterRelease(TIMInterpreter *interp);

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim);
int TIMInterpreterIsRunning(const TIMInterpreter *interp);
int TIMInterpreterUpdate(TIMInterpreter *interp);
void TIMInterpreterButtonClicked(TIMInterpreter *interp, int buttonIndex);
