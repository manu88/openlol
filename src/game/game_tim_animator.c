#include "game_tim_animator.h"
#include "animator.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void callbackTIM_WSAInit(TIMInterpreter *interp, uint16_t index,
                                const char *wsaFile, int x, int y,
                                int offscreen, int flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameFile f = {0};
  printf("----> GameTimAnimator load wsa file '%s' index %i offscreen=%i\n",
         wsaFile, index, offscreen);
  assert(GameEnvironmentGetFileWithExt(&f, wsaFile, "WSA"));
  AnimatorInitWSA(&gameCtx->animator, f.buffer, f.bufferSize, x, y, offscreen,
                  flags);
}

static void callbackTIM_WSADisplayFrame(TIMInterpreter *interp, int animIndex,
                                        int frame) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameTimInterpreter *animator = &gameCtx->timInterpreter;
  assert(animator);

  printf("GameTimAnimator: callbackWSADisplayFrame animIndex=%i fram=%i x=%i "
         "y=%i\n",
         animIndex, frame, animator->animator->wsaX, animator->animator->wsaY);
  if (frame >= animator->animator->wsa.header.numFrames + 1) {
    printf("WSADisplayFrame: unimplemented WSA loop, setting frame to 0\n");
    frame = 0;
  }
  WSAHandleGetFrame(&animator->animator->wsa, frame,
                    animator->animator->wsaFrameBuffer,
                    animator->animator->wsaFlags & WSA_XOR);
  assert(animator->animator->wsaFrameBuffer);
  AnimatorRenderWSAFrame(animator->animator);
}

static void callbackTIM_FadeClearWindow(TIMInterpreter *interp,
                                        uint16_t param) {
  printf("GameTimAnimator: callbackFadeClearWindow param=%x\n", param);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->fadeOutFrames = 10;
}

static uint16_t callbackTIM_GiveItem(TIMInterpreter *interp, uint16_t p0,
                                     uint16_t p1, uint16_t p2) {
  printf("callbackTIM_GiveItem: p0=%x p1=%x p2=%x\n", p0, p1, p2);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return GameContextAddItemToInventory(gameCtx, p0);
}

static void callbackTIM_WSARelease(TIMInterpreter *interp, int index) {
  printf("GameTimAnimator: callbackWSARelease index=%i\n", index);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  WSAHandleRelease(&gameCtx->animator.wsa);
}

static void callbackTIM_PlayDialogue(TIMInterpreter *interp, uint16_t stringId,
                                     int argc, const uint16_t *argv) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;

  GameContextGetString(gameCtx, stringId, gameCtx->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  gameCtx->dialogText = gameCtx->dialogTextBuffer;
}

static void callbackTIM_ShowDialogButtons(TIMInterpreter *interp,
                                          uint16_t functionId,
                                          const uint16_t buttonStrIds[3]) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("GameTimAnimator ShowDialogButtons  %X %X %X\n", buttonStrIds[0],
         buttonStrIds[1], buttonStrIds[2]);

  for (int i = 0; i < 3; i++) {
    if (buttonStrIds[i] == 0XFFFF) {
      break;
    }
    if (gameCtx->buttonText[i] == NULL) {
      gameCtx->buttonText[i] = malloc(16);
    }
    GameContextGetString(gameCtx, buttonStrIds[i], gameCtx->buttonText[i], 16);
    printf("button %i = %s\n", i, gameCtx->buttonText[i]);
  }
}

static void callbackTIM_InitSceneDialog(TIMInterpreter *interp,
                                        int controlMode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("GameTimAnimator callbackInitSceneDialog controlMode=%X\n",
         controlMode);
  GameContextInitSceneDialog(gameCtx);
}

static void callbackTIM_RestoreAfterSceneDialog(TIMInterpreter *interp,
                                                int controlMode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("GameTimAnimator callbackInitSceneDialog controlMode=%X\n",
         controlMode);
  GameContextCleanupSceneDialog(gameCtx);
}

void GameTimInterpreterInit(GameTimInterpreter *animator, Animator *animator_) {
  memset(animator, 0, sizeof(GameTimInterpreter));
  animator->animator = animator_;
  TIMInterpreterInit(&animator->timInterpreter);
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSAInit =
      callbackTIM_WSAInit;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSADisplayFrame =
      callbackTIM_WSADisplayFrame;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_PlayDialogue =
      callbackTIM_PlayDialogue;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_ShowDialogButtons =
      callbackTIM_ShowDialogButtons;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_InitSceneDialog =
      callbackTIM_InitSceneDialog;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_WSARelease =
      callbackTIM_WSARelease;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_FadeClearWindow =
      callbackTIM_FadeClearWindow;
  animator->timInterpreter.callbacks.TIMInterpreterCallbacks_GiveItem =
      callbackTIM_GiveItem;
  animator->timInterpreter.callbacks
      .TIMInterpreterCallbacks_RestoreAfterSceneDialog =
      callbackTIM_RestoreAfterSceneDialog;
}

void GameTimInterpreterRelease(GameTimInterpreter *animator) {
  TIMInterpreterRelease(&animator->timInterpreter);
  AnimatorRelease(animator->animator);
}

void GameTimInterpreterLoadTim(GameTimInterpreter *animator, uint16_t scriptId,
                               const char *file) {
  GameFile f = {0};
  assert(GameEnvironmentGetFileWithExt(&f, file, "TIM"));
  assert(TIMHandleFromBuffer(&animator->tim[scriptId], f.buffer, f.bufferSize));
}

void GameTimInterpreterRunTim(GameTimInterpreter *animator, uint16_t scriptId) {
  printf("GameTimAnimatorRunTim start %x\n", scriptId);
  TIMInterpreterStart(&animator->timInterpreter, &animator->tim[scriptId]);
}

void GameTimInterpreterReleaseTim(GameTimInterpreter *animator,
                                  uint16_t scriptId) {
  TIMHandleInit(&animator->tim[scriptId]);
}

int GameTimInterpreterRender(GameTimInterpreter *animator) {
  printf("GameTimAnimatorRender\n");
  // int timeToWait = 0;
  if (TIMInterpreterIsRunning(&animator->timInterpreter)) {
    TIMInterpreterUpdate(&animator->timInterpreter);
  } else {
    printf("TIM anim is done\n");
    return 0;
  }
  return 1;
}

void GameTimAnimatorSetupPart(GameTimInterpreter *animator, uint16_t animIndex,
                              uint16_t part, uint16_t firstFrame,
                              uint16_t lastFrame, uint16_t cycles,
                              uint16_t nextPart, uint16_t partDelay,
                              uint16_t field, uint16_t sfxIndex,
                              uint16_t sfxFrame) {
  printf("GameTimAnimatorSetupPart\n");
}

void GameTimAnimatorPlayPart(GameTimInterpreter *animator, uint16_t animIndex,
                             uint16_t firstFrame, uint16_t lastFrame,
                             uint16_t delay) {
  printf("GameTimAnimatorPlayPart %x\n", animIndex);
  assert(animator->animator->wsa.originalBuffer);
}
