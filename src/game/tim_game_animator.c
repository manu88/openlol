#include "tim_game_animator.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "game_envir.h"
#include "geometry.h"
#include "renderer.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void callbackWSAInit(TIMInterpreter *interp, const char *wsaFile, int x,
                            int y, int offscreen, int flags) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  GameFile f = {0};
  printf("----> GameTimAnimator load wsa file '%s'\n", wsaFile);
  assert(GameEnvironmentGetFileWithExt(&f, wsaFile, "WSA"));
  WSAHandleFromBuffer(&animator->wsa, f.buffer, f.bufferSize);
  if (animator->wsa.header.palette == NULL) {
    printf("WSA has no palette, using the game level one\n");
    animator->wsa.header.palette = animator->defaultPalette;
  }
  animator->wsaX = x;
  animator->wsaY = y;
  animator->wsaFlags = flags;
  printf("WSA w=%i h=%i\n", animator->wsa.header.width,
         animator->wsa.header.height);
  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
  animator->wsaFrameBuffer =
      malloc(animator->wsa.header.width * animator->wsa.header.height);
  memset(animator->wsaFrameBuffer, 0,
         animator->wsa.header.width * animator->wsa.header.height);
  assert(animator->wsaFrameBuffer);
}

static void renderWSAFrame(GameTimAnimator *animator, const uint8_t *imgData,
                           size_t dataSize, const uint8_t *paletteBuffer, int w,
                           int h) {
  assert(animator->pixBuf);
  void *data;
  int pitch;
  SDL_LockTexture(animator->pixBuf, NULL, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      int offset = (w * y) + x;
      if (offset >= dataSize) {
        printf("Offset %i >= %zu\n", offset, dataSize);
      }
      assert(offset < dataSize);
      uint8_t paletteIdx = *(imgData + offset);

      uint8_t r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
      uint8_t g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
      uint8_t b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      uint32_t *row =
          (unsigned int *)((char *)data + pitch * (animator->wsaY + y));
      row[animator->wsaX + x] = 0XFF000000 + (r << 0X10) + (g << 0X8) + b;
    }
  }
  SDL_UnlockTexture(animator->pixBuf);
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int frameIndex,
                                    int frame) {
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);

  printf("GameTimAnimator: callbackWSADisplayFrame frameIndex=%i fram=%i x=%i "
         "y=%i\n",
         frameIndex, frame, animator->wsaX, animator->wsaY);
  WSAHandleGetFrame(&animator->wsa, frame, animator->wsaFrameBuffer,
                    animator->wsaFlags & WSA_XOR);
  assert(animator->wsaFrameBuffer);
  size_t fullSize = animator->wsa.header.width * animator->wsa.header.height;
  renderWSAFrame(animator, animator->wsaFrameBuffer, fullSize,
                 animator->wsa.header.palette, animator->wsa.header.width,
                 animator->wsa.header.height);
}

static void callbackWSARelease(TIMInterpreter *interp, int index) {
  printf("GameTimAnimator: callbackWSARelease index=%i\n", index);
  GameTimAnimator *animator = (GameTimAnimator *)interp->callbackCtx;
  assert(animator);
  WSAHandleRelease(&animator->wsa);
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

void GameTimAnimatorInit(GameTimAnimator *animator, SDL_Texture *pixBuf) {
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

  animator->pixBuf = pixBuf;
}

void GameTimAnimatorRelease(GameTimAnimator *animator) {
  TIMInterpreterRelease(&animator->timInterpreter);
  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
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

int GameTimAnimatorRender(GameTimAnimator *animator) {
  printf("GameTimAnimatorRender\n");
  int timeToWait = 0;
  if (TIMInterpreterIsRunning(&animator->timInterpreter)) {
    timeToWait = TIMInterpreterUpdate(&animator->timInterpreter);
  } else {
    printf("TIM anim is done\n");
    return 0;
  }

  return 1;
}
