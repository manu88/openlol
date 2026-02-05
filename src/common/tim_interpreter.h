#pragma once
#include "formats/format_tim.h"
#include <stddef.h>
#include <stdint.h>

typedef struct _TIMInterpreter TIMInterpreter;

typedef struct {
  void (*TIMInterpreterCallbacks_WSAInit)(TIMInterpreter *interp,
                                          uint16_t index, const char *wsaFile,
                                          int x, int y, int offscreen,
                                          int flags);
  void (*TIMInterpreterCallbacks_WSARelease)(TIMInterpreter *interp, int index);
  void (*TIMInterpreterCallbacks_WSADisplayFrame)(TIMInterpreter *interp,
                                                  int animIndex, int frame);
  void (*TIMInterpreterCallbacks_PlayDialogue)(TIMInterpreter *interp,
                                               uint16_t strId, int argc,
                                               const uint16_t *argv);
  void (*TIMInterpreterCallbacks_CharChat)(TIMInterpreter *interp,
                                           uint16_t charId, uint16_t mode,
                                           uint16_t stringId);
  void (*TIMInterpreterCallbacks_ShowDialogButtons)(
      TIMInterpreter *interp, uint16_t functionId,
      const uint16_t buttonStrIds[3]);
  void (*TIMInterpreterCallbacks_InitSceneDialog)(TIMInterpreter *interp,
                                                  int controlMode);

  void (*TIMInterpreterCallbacks_RestoreAfterSceneDialog)(
      TIMInterpreter *interp, int controlMode);

  void (*TIMInterpreterCallbacks_FadeClearWindow)(TIMInterpreter *interp,
                                                  uint16_t param);
  uint16_t (*TIMInterpreterCallbacks_GiveItem)(TIMInterpreter *interp,
                                               uint16_t param0, uint16_t param1,
                                               uint16_t param2);
  void (*TIMInterpreterCallbacks_CopyPage)(TIMInterpreter *interp,
                                           uint16_t srcX, uint16_t srcY,
                                           uint16_t destX, uint16_t destY,
                                           uint16_t w, uint16_t h,
                                           uint16_t srcPage, uint16_t dstPage);
  void (*TIMInterpreterCallbacks_PlaySoundFX)(TIMInterpreter *interp,
                                              uint16_t soundId);
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

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim);
int TIMInterpreterIsRunning(const TIMInterpreter *interp);
int TIMInterpreterUpdate(TIMInterpreter *interp);
void TIMInterpreterButtonClicked(TIMInterpreter *interp, int buttonIndex);
