#include "tim_game_animator.h"
#include "formats/format_tim.h"
#include "game_envir.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void callbackWSAInit(TIMInterpreter *interp, const char *wsaFile, int x,
                            int y, int offscreen, int flags) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int frameIndex,
                                    int frame) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  printf("GameTimAnimator: callbackWSADisplayFrame frameIndex=%i fram=%i\n",
         frameIndex, frame);
}

static void callbackPlayDialogue(TIMInterpreter *interp, uint16_t stringId,
                                 int argc, const uint16_t *argv) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  printf("GameTimAnimator callbackPlayDialogue stringId=%i argc=%i\n", stringId,
         argc);
}

static void callbackShowButtons(TIMInterpreter *interp, uint16_t functionId,
                                const uint16_t buttonStrIds[3]) {
  printf("GameTimAnimator callbackShowButtons  %X %X %X\n", buttonStrIds[0],
         buttonStrIds[1], buttonStrIds[2]);
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
}

static void callbackInitSceneDialog(TIMInterpreter *interp, int controlMode) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  printf("GameTimAnimator callbackInitSceneDialog controlMode=%X\n",
         controlMode);
}

static void callbackWSARelease(TIMInterpreter *interp, int index) {
  printf("GameTimAnimator: callbackWSARelease index=%i\n", index);
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  WSAHandleRelease(&animator->wsa);
}

void GameTimAnimatorInit(GameTimAnimator *animator) {
  memset(animator, 0, sizeof(GameTimAnimator));
  TIMInterpreterInit(&animator->timInterpreter);
  animator->timInterpreter.callbackCtx = animator;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSAInit =
      callbackWSAInit;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSADisplayFrame =
      callbackWSADisplayFrame;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_PlayDialogue =
      callbackPlayDialogue;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_ShowButtons =
      callbackShowButtons;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_InitSceneDialog =
      callbackInitSceneDialog;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSARelease =
      callbackWSARelease;
}

void GameTimAnimatorLoadTim(GameTimAnimator *animator, uint16_t scriptId,
                            const char *file) {
  GameFile f = {0};
  assert(GameEnvironmentGetFileWithExt(&f, file, "TIM"));
  assert(TIMHandleFromBuffer(&animator->tim[scriptId], f.buffer, f.bufferSize));
}

void GameTimAnimatorRunTim(GameTimAnimator *animator, uint16_t scriptId) {
  printf("GameTimAnimatorRunTim start %x\n", scriptId);
  TIMInterpreterStart(&animator->timInterpreter, &animator->tim[scriptId]);
}

void GameTimAnimatorReleaseTim(GameTimAnimator *animator, uint16_t scriptId) {
  TIMHandleInit(&animator->tim[scriptId]);
}

void GameTimAnimatorRender(GameTimAnimator *animator) {
  printf("GameTimAnimatorRender\n");
}
