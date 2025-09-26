#include "tim_animator.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_render.h"
#include "bytes.h"
#include "format_lang.h"
#include "format_wsa.h"
#include "renderer.h"
#include <_string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xlocale/_stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define ANIM_WIDTH 300
#define ANIM_HEIGHT 200
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
      SDL_CreateRenderer(animator->window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  animator->pixBuf =
      SDL_CreateTexture(animator->renderer, SDL_PIXELFORMAT_XRGB8888,
                        SDL_TEXTUREACCESS_STREAMING, ANIM_WIDTH, ANIM_HEIGHT);
  if (animator->pixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
  }
  return 1;
}

const char *wsaFlags(int flags) {
  static char s[7] = "000000";
  if (flags & WSA_OFFSCREEN_DECODE) {
    s[0] = '1';
  }
  if (flags & WSA_NO_LAST_FRAME) {
    s[1] = '1';
  }
  if (flags & WSA_NO_FIRST_FRAME) {
    s[2] = '1';
  }
  if (flags & WSA_FLIPPED) {
    s[3] = '1';
  }
  if (flags & WSA_HAS_PALETTE) {
    s[4] = '1';
  }
  if (flags & WSA_XOR) {
    s[5] = '1';
  }
  return s;
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int frameIndex,
                                    int frame);

static void callbackWSAInit(TIMInterpreter *interp, const char *wsaFile, int x,
                            int y, int offscreen, int flags) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator: callbackWSAInit wsa file='%s' x=%i y=%i offscreen=%i "
         "flags=%X %s\n",
         wsaFile, x, y, offscreen, flags, wsaFlags(flags));

  size_t fileSize = 0;
  size_t readSize = 0;
  char filePath[16] = "";
  snprintf(filePath, sizeof(filePath), "%s.wsa", wsaFile);
  uint8_t *buffer = readBinaryFile(filePath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return;
  }
  WSAHandleInit(&animator->wsa);
  WSAHandleFromBuffer(&animator->wsa, buffer, readSize);
  printf("WSAHandle created\n");
  callbackWSADisplayFrame(interp, 0, 0);
}

static void callbackWSARelease(TIMInterpreter *interp, int index) {
  printf("TIMAnimator: callbackWSARelease index=%i\n", index);
}

static void renderWSAFrame(TIMAnimator *animator, const uint8_t *imgData,
                           size_t dataSize, const uint8_t *paletteBuffer, int w,
                           int h, int doXOR) {
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

      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      if (doXOR) {
        row[x] ^= 0XFF + (r << 0X10) + (g << 0X8) + b;
      } else {
        row[x] = 0XFF + (r << 0X10) + (g << 0X8) + b;
      }
    }
  }
  SDL_UnlockTexture(animator->pixBuf);
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int frameIndex,
                                    int frame) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator: callbackWSADisplayFrame frameIndex=%i fram=%i\n",
         frameIndex, frame);
  uint8_t *frameData = WSAHandleGetFrame(&animator->wsa, frame);
  assert(frameData);
  size_t fullSize = animator->wsa.header.width * animator->wsa.header.height;
  renderWSAFrame(animator, frameData, fullSize, animator->wsa.header.palette,
                 animator->wsa.header.width, animator->wsa.header.height,
                 frame != 0);
  free(frameData);
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

static void callbackShowButtons(TIMInterpreter *interp, uint16_t functionId,
                                const uint16_t buttonStrIds[3]) {
  printf("TIMAnimator callbackShowButtons  %X %X %X\n", buttonStrIds[0],
         buttonStrIds[1], buttonStrIds[2]);
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  for (int i = 0; i < 3; i++) {
    if (buttonStrIds[i] == 0XFFFF) {
      break;
    }
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

static void callbackInitSceneDialog(TIMInterpreter *interp, int controlMode) {
  printf("TIMAnimator callbackInitSceneDialog controlMode=%X\n", controlMode);
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
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_InitSceneDialog =
      callbackInitSceneDialog;
  animator->_interpreter.callbacks.TIMInterpreterCallbacks_WSARelease =
      callbackWSARelease;
}
void TIMAnimatorRelease(TIMAnimator *animator) {
  TIMInterpreterRelease(&animator->_interpreter);
  if (animator->window) {
    SDL_DestroyWindow(animator->window);
  }
  SDL_DestroyRenderer(animator->renderer);
  WSAHandleRelease(&animator->wsa);
}

static void mainLoop(TIMAnimator *animator, uint32_t ms) {
  int quit = 0;
  while (!quit) {
    SDL_Event e;
    SDL_WaitEventTimeout(&e, 30);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
      case SDLK_SPACE:
        if (TIMInterpreterIsRunning(&animator->_interpreter)) {
          TIMInterpreterUpdate(&animator->_interpreter, ms);
        } else {
          printf("TIM anim is done\n");
        }
        ms += 30;

        break;
      }
    }
    SDL_RenderClear(animator->renderer);
    SDL_RenderCopy(animator->renderer, animator->pixBuf, NULL, NULL);
    SDL_RenderPresent(animator->renderer);
  } // end while
  SDL_Quit();
}

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim) {
  if (initSDLStuff(animator) == 0) {
    return 0;
  }
  printf("TIMAnimatorRunAnim\n");
  animator->tim = tim;
  uint32_t ms = 0;
  TIMInterpreterStart(&animator->_interpreter, tim, ms);
  mainLoop(animator, ms);
  return 1;
}
