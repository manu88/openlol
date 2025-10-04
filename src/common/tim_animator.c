#include "tim_animator.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_render.h"
#include "bytes.h"
#include "formats/format_lang.h"
#include "formats/format_wsa.h"
#include "renderer.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale/_stdio.h>

#define ANIM_WIDTH 300
#define ANIM_HEIGHT 200

static int initSDLStuff(TIMAnimator *animator) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 0;
  }
  if (TTF_Init() == -1) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }
  animator->window =
      SDL_CreateWindow("TIM animator", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
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

  animator->font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 18);
  if (!animator->font) {
    printf("unable to create font\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }
  return 1;
}

static void renderStatLine(TIMAnimator *animator, const char *line, int x,
                           int y) {
  animator->textSurface = TTF_RenderUTF8_Solid(animator->font, line,
                                               (SDL_Color){255, 255, 255, 255});

  animator->textTexture =
      SDL_CreateTextureFromSurface(animator->renderer, animator->textSurface);
  SDL_Rect dstrect = {x, y, animator->textSurface->w, animator->textSurface->h};
  SDL_RenderCopy(animator->renderer, animator->textTexture, NULL, &dstrect);
  SDL_DestroyTexture(animator->textTexture);
  SDL_FreeSurface(animator->textSurface);
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
  animator->wsaFlags = flags;
  animator->animXOffset = x;
  animator->animYOffset = y;
  size_t fileSize = 0;
  size_t readSize = 0;
  char filePath[16] = "";
  snprintf(filePath, sizeof(filePath), "%s.WSA", wsaFile);
  uint8_t *buffer = readBinaryFile(filePath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return;
  }
  WSAHandleInit(&animator->wsa);
  WSAHandleFromBuffer(&animator->wsa, buffer, readSize);

  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
  animator->wsaFrameBuffer =
      malloc(animator->wsa.header.width * animator->wsa.header.height);
  memset(animator->wsaFrameBuffer, 0,
         animator->wsa.header.width * animator->wsa.header.height);
  assert(animator->wsaFrameBuffer);
  printf("WSAHandle created\n");
}

static void callbackWSARelease(TIMInterpreter *interp, int index) {
  printf("TIMAnimator: callbackWSARelease index=%i\n", index);
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  WSAHandleRelease(&animator->wsa);
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

      uint8_t r = VGA6To8(paletteBuffer[(paletteIdx * 3) + 0]);
      uint8_t g = VGA6To8(paletteBuffer[(paletteIdx * 3) + 1]);
      uint8_t b = VGA6To8(paletteBuffer[(paletteIdx * 3) + 2]);
      if (r != 255 && b != 255 && b != 0) {
        int xx = x + animator->animXOffset;
        int yy = y + animator->animYOffset;
        uint32_t *row = (unsigned int *)((char *)data + pitch * yy);

        row[xx] = 0XFF + (r << 0X10) + (g << 0X8) + b;
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

  WSAHandleGetFrame(&animator->wsa, frame, animator->wsaFrameBuffer, 1);
  assert(animator->wsaFrameBuffer);
  size_t fullSize = animator->wsa.header.width * animator->wsa.header.height;
  renderWSAFrame(animator, animator->wsaFrameBuffer, fullSize,
                 animator->wsa.header.palette, animator->wsa.header.width,
                 animator->wsa.header.height, animator->wsaFlags & WSA_XOR);
}

static void callbackPlayDialogue(TIMInterpreter *interp, uint16_t stringId,
                                 int argc, const uint16_t *argv) {
  TIMAnimator *animator = (TIMAnimator *)interp->callbackCtx;
  assert(animator);
  printf("TIMAnimator callbackPlayDialogue stringId=%i argc=%i\n", stringId,
         argc);
  if (argc) {
    for (int i = 0; i < argc; i++) {
      printf("0X%X ", argv[i]);
    }
    printf("\n");
  }
  uint8_t useLevelFile = 0;
  int realId = LangGetString(stringId, &useLevelFile);
  if (!useLevelFile) {
    assert(0); // to implement :)
  }
  if (realId != -1 && animator->lang) {
    if (animator->currentDialog == NULL) {
      animator->currentDialog = malloc(1024);
      assert(animator->currentDialog);
      memset(animator->currentDialog, 0, 1024);
    }
    printf("Dialog id %i\n", realId);
    LangHandleGetString(animator->lang, realId, animator->currentDialog, 1024);
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
    printf("LangGetString %i %X\n", i, buttonStrIds[i]);
    int realId = LangGetString(buttonStrIds[i], &useLevelFile);
    assert(realId != -1);
    if (!useLevelFile) {
      assert(0); // to implement :)
    }
    if (realId != -1 && animator->lang) {
      if (animator->buttonText[i] == NULL) {
        animator->buttonText[i] = malloc(16);
        assert(animator->buttonText[i]);
        memset(animator->buttonText[i], 0, 16);
      }
      printf("realID = %x\n", realId);
      LangHandleGetString(animator->lang, realId, animator->buttonText[i], 16);
      printf("Dialogue Button %i: '%s'\n", i, animator->buttonText[i]);
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

  SDL_DestroyTexture(animator->pixBuf);

  if (animator->font) {
    TTF_CloseFont(animator->font);
  }
  if (animator->currentDialog) {
    free(animator->currentDialog);
  }
  for (int i = 0; i < 3; i++) {
    if (animator->buttonText[i]) {
      free(animator->buttonText[i]);
    }
  }

  WSAHandleRelease(&animator->wsa);
  if (animator->wsaFrameBuffer) {
    free(animator->wsaFrameBuffer);
  }
}

static void hideButton(TIMAnimator *animator, int index) {
  if (animator->buttonText[index]) {
    free(animator->buttonText[index]);
    animator->buttonText[index] = NULL;
  }
}

static void mainLoop(TIMAnimator *animator) {
  int quit = 0;
  int timeToWait = 0;

  int stepNext = 0;
  while (!quit) {
    SDL_Event e;
    SDL_WaitEventTimeout(&e, timeToWait);
    SDL_RenderClear(animator->renderer);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
      case SDLK_1:
        printf("Button1\n");
        animator->buttonsState[0] = 1;
        TIMInterpreterButtonClicked(&animator->_interpreter, 0);
        hideButton(animator, 0);
        break;
      case SDLK_2:
        printf("Button2\n");
        animator->buttonsState[1] = 1;
        break;
      case SDLK_3:
        printf("Button3\n");
        animator->buttonsState[2] = 1;
        break;
      case SDLK_SPACE:
        stepNext = 1;
        break;
      }
    }
    if (stepNext) {
      if (TIMInterpreterIsRunning(&animator->_interpreter)) {
        timeToWait = TIMInterpreterUpdate(&animator->_interpreter) * 10;
      } else {
        printf("TIM anim is done\n");
        break;
      }
      stepNext = 0;
    }
    SDL_RenderCopy(animator->renderer, animator->pixBuf, NULL, NULL);
    if (animator->currentDialog) {
      renderStatLine(animator, animator->currentDialog, 10, 300);
    }
    for (int i = 0; i < 3; i++) {
      if (animator->buttonText[i]) {
        renderStatLine(animator, animator->buttonText[i], 100, 350);
      }
    }
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
  TIMInterpreterStart(&animator->_interpreter, tim);
  mainLoop(animator);
  return 1;
}
