#include "tim_animator.h"
#include "SDL_keycode.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "bytes.h"
#include "format_lang.h"
#include "format_wsa.h"
#include "renderer.h"
#include "tim_interpreter.h"
#include <_string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xlocale/_stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

static int initSDLStuff(TIMAnimator *animator) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 0;
  }
  animator->window = SDL_CreateWindow("TIM animator", SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                      SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (animator->window == NULL) {
    return 0;
  }

  animator->renderer =
      SDL_CreateRenderer(animator->window, -1, SDL_RENDERER_ACCELERATED);

  return 1;
}

static void callbackWSAInit(TIMInterpreter *interp, const char *wsaFile) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator: callbackWSAInit '%s'\n", wsaFile);

  size_t fileSize = 0;
  size_t readSize = 0;
  char filePath[12] = "";
  snprintf(filePath, 12, "%s.wsa", wsaFile);
  uint8_t *buffer = readBinaryFile(filePath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return;
  }
  WSAHandleInit(&animator->wsa);
  WSAHandleFromBuffer(&animator->wsa, buffer, readSize);
  printf("WSAHandle created\n");
}

static void renderCPSImage(SDL_Renderer *renderer, const uint8_t *imgData,
                           size_t dataSize, const uint8_t *paletteBuffer, int w,
                           int h) {
  printf("renderCPSImage\n");
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      int offset = (w * y) + x;
      if (offset >= dataSize) {
        printf("Offset %i >= %zu\n", offset, dataSize);
      }
      assert(offset < dataSize);
      uint8_t paletteIdx = *(imgData + offset);
      uint8_t r;
      uint8_t g;
      uint8_t b;
      if (paletteBuffer) {
        r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
        g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
        b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      } else {
        r = paletteIdx;
        g = paletteIdx;
        b = paletteIdx;
      }

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_Rect rect = {.x = x * 2, .y = y * 2, .w = 2, .h = 2};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int frameIndex,
                                    int frame) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator: callbackWSADisplayFrame frameIndex=%i fram=%i\n",
         frameIndex, frame);
  uint8_t *frame0Data = WSAHandleGetFrame(&animator->wsa, frameIndex);
  assert(frame0Data);
  size_t fullSize = animator->wsa.header.width * animator->wsa.header.height;
  renderCPSImage(animator->renderer, frame0Data, fullSize,
                 animator->wsa.header.palette, animator->wsa.header.width,
                 animator->wsa.header.height);
  free(frame0Data);
}

static char tempStr[1024];
static void callbackPlayDialogue(TIMInterpreter *interp, uint16_t stringId) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator callbackPlayDialogue stringId=%i", stringId);
  uint8_t useLevelFile = 0;
  int realId = LangGetString(stringId, &useLevelFile);
  if (!useLevelFile) {
    assert(0); // to implement :)
  }
  if (realId != -1 && animator->lang) {
    LangHandleGetString(animator->lang, realId, tempStr, sizeof(tempStr));
    printf("Dialogue: '%s'\n", tempStr);
  }
}

static void callbackShowButtons(TIMInterpreter *interp,
                                uint16_t buttonStrIds[3], int numButtons) {
  printf("TIMAnimator callbackShowButtons numButtons=%i %X %X %X\n", numButtons,
         buttonStrIds[0], buttonStrIds[1], buttonStrIds[2]);
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  for (int i = 0; i < numButtons; i++) {
    assert(buttonStrIds[i] != 0XFFFF);
    uint8_t useLevelFile = 0;
    int realId = LangGetString(buttonStrIds[i], &useLevelFile);
    if (!useLevelFile) {
      assert(0); // to implement :)
    }
    if (realId != -1 && animator->lang) {
      LangHandleGetString(animator->lang, realId, tempStr, sizeof(tempStr));
      printf("Dialogue Button %i: '%s'\n", i, tempStr);
    }
  }
}

void TIMAnimatorInit(TIMAnimator *animator) {
  memset(animator, 0, sizeof(TIMAnimator));
  TIMInterpreterInit(&animator->_interpreter);
  animator->_interpreter.callbackCtx = animator;
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_WSAInit =
      callbackWSAInit;
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_WSADisplayFrame =
      callbackWSADisplayFrame;
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_PlayDialogue =
      callbackPlayDialogue;
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_ShowButtons =
      callbackShowButtons;
}
void TIMAnimatorRelease(TIMAnimator *animator) {
  TIMInterpreterRelease(&animator->_interpreter);
  if (animator->window) {
    SDL_DestroyWindow(animator->window);
  }
  SDL_DestroyRenderer(animator->renderer);
  WSAHandleRelease(&animator->wsa);
}

static void mainLoop(TIMAnimator *animator) {
  SDL_RenderClear(animator->renderer);
  int quit = 0;
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
      case SDLK_SPACE:
        if (TIMInterpreterIsRunning(&animator->_interpreter)) {
          TIMInterpreterUpdate(&animator->_interpreter);
        } else {
          printf("TIM anim is done\n");
        }
        printf("update\n");
        SDL_RenderPresent(animator->renderer);
      }
    }
  }
}

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim) {
  if (initSDLStuff(animator) == 0) {
    return 0;
  }
  printf("TIMAnimatorRunAnim\n");
  animator->tim = tim;
  TIMInterpreterStart(&animator->_interpreter, tim);
  mainLoop(animator);
  return 1;
}
