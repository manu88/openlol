#include "game_tim_animator.h"
#include "animator.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "game.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "game_strings.h"
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
  GameTimInterpreter *timInterp = &gameCtx->timInterpreter;
  assert(timInterp);

  printf("GameTimAnimator: callbackWSADisplayFrame animIndex=%i frame=%i x=%i "
         "y=%i\n",
         animIndex, frame, timInterp->animator->wsaX,
         timInterp->animator->wsaY);
  if (frame >= timInterp->animator->wsa.header.numFrames + 1) {
    printf("WSADisplayFrame: unimplemented WSA loop, setting frame to 0\n");
    frame = 0;
  }
  WSAHandleGetFrame(&timInterp->animator->wsa, frame,
                    timInterp->animator->wsaFrameBuffer,
                    timInterp->animator->wsaFlags & WSA_XOR);
  assert(timInterp->animator->wsaFrameBuffer);
  AnimatorRenderWSAFrame(timInterp->animator);
}

static void callbackTIM_FadeClearWindow(TIMInterpreter *interp,
                                        uint16_t param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("GameTimAnimator: callbackFadeClearWindow param=%x\n", param);
  GameDoSceneFade(gameCtx, 10);
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

static void callbackTIM_CharChat(TIMInterpreter *interp, uint16_t charId,
                                 uint16_t mode, uint16_t stringId) {
  printf(
      "GameTimAnimator: callbackTIM_CharChat charId=%x mode=%x stringId=%x\n",
      charId, mode, stringId);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextGetString(gameCtx, stringId, gameCtx->display->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  GameContextSetDialogF(gameCtx, stringId);
  if (charId == 1) {
    charId = gameCtx->selectedChar;
  }
  GameContextPlayDialogSpeech(gameCtx, 0, stringId);
}

static void callbackTIM_PlayDialogue(TIMInterpreter *interp, uint16_t stringId,
                                     int argc, const uint16_t *argv) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;

  GameContextGetString(gameCtx, stringId, gameCtx->display->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  GameContextSetDialogF(gameCtx, stringId);
  GameContextPlayDialogSpeech(gameCtx, 0, stringId);
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

static void callbackTIM_PlaySoundFX(TIMInterpreter *interp, uint16_t soundId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextPlaySoundFX(gameCtx, soundId);
}

static void callbackTIM_CopyPage(TIMInterpreter *interp, uint16_t srcX,
                                 uint16_t srcY, uint16_t destX, uint16_t destY,
                                 uint16_t w, uint16_t h, uint16_t srcPage,
                                 uint16_t dstPage) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameCopyPage(gameCtx, srcX, srcY, destX, destY, w, h, srcPage, dstPage);
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

void GameTimInterpreterInit(GameTimInterpreter *timInterpreter,
                            Animator *animator) {
  memset(timInterpreter, 0, sizeof(GameTimInterpreter));
  timInterpreter->animator = animator;
  TIMInterpreterInit(&timInterpreter->timInterpreter);
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_WSAInit =
      callbackTIM_WSAInit;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_WSADisplayFrame = callbackTIM_WSADisplayFrame;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_PlayDialogue = callbackTIM_PlayDialogue;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_ShowDialogButtons =
      callbackTIM_ShowDialogButtons;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_InitSceneDialog = callbackTIM_InitSceneDialog;
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_WSARelease =
      callbackTIM_WSARelease;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_FadeClearWindow = callbackTIM_FadeClearWindow;
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_GiveItem =
      callbackTIM_GiveItem;
  timInterpreter->timInterpreter.callbacks
      .TIMInterpreterCallbacks_RestoreAfterSceneDialog =
      callbackTIM_RestoreAfterSceneDialog;
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_CharChat =
      callbackTIM_CharChat;
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_CopyPage =
      callbackTIM_CopyPage;
  timInterpreter->timInterpreter.callbacks.TIMInterpreterCallbacks_PlaySoundFX =
      callbackTIM_PlaySoundFX;
}

void GameTimInterpreterRelease(GameTimInterpreter *animator) {
  TIMInterpreterRelease(&animator->timInterpreter);
  AnimatorRelease(animator->animator);
}

void GameTimInterpreterLoadTim(GameTimInterpreter *timInterp, uint16_t scriptId,
                               const char *file) {
  GameFile f = {0};
  assert(GameEnvironmentGetFileWithExt(&f, file, "TIM"));
  assert(
      TIMHandleFromBuffer(&timInterp->tim[scriptId], f.buffer, f.bufferSize));
}

void GameTimInterpreterRunTim(GameTimInterpreter *timInterp,
                              uint16_t scriptId) {
  printf("GameTimAnimatorRunTim start %x\n", scriptId);
  TIMInterpreterStart(&timInterp->timInterpreter, &timInterp->tim[scriptId]);
}

void GameTimInterpreterReleaseTim(GameTimInterpreter *timInterp,
                                  uint16_t scriptId) {
  TIMHandleInit(&timInterp->tim[scriptId]);
}

int GameTimInterpreterRender(GameTimInterpreter *timInterp) {
  printf("GameTimAnimatorRender\n");
  // int timeToWait = 0;
  if (TIMInterpreterIsRunning(&timInterp->timInterpreter)) {
    TIMInterpreterUpdate(&timInterp->timInterpreter);
  } else {
    printf("TIM anim is done\n");
    return 0;
  }
  return 1;
}

void GameTimAnimatorSetupPart(GameTimInterpreter *timInterp, uint16_t animIndex,
                              uint16_t part, uint16_t firstFrame,
                              uint16_t lastFrame, uint16_t cycles,
                              uint16_t nextPart, uint16_t partDelay,
                              uint16_t field, uint16_t sfxIndex,
                              uint16_t sfxFrame) {
  printf("GameTimAnimatorSetupPart\n");
}

void GameTimAnimatorPlayPart(GameTimInterpreter *timInterp, uint16_t animIndex,
                             uint16_t firstFrame, uint16_t lastFrame,
                             uint16_t delay) {
  printf("GameTimAnimatorPlayPart %x\n", animIndex);
  assert(timInterp->animator->wsa.originalBuffer);
}
