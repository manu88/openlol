#include "tim.h"
#include "SDL_events.h"
#include "display.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_strings.h"
#include "game_tim_animator.h"
#include "geometry.h"
#include "tim_interpreter.h"
#include "ui.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TIM_NUM_ANIMATIONS 4
#define WSA_NUM_ANIMATIONS 6

#define TEXT_BUFFER_SIZE 128
static char textBuffer[128] = "";

typedef struct {
  TIMHandle tim;
} TIMScript;

typedef struct {
  WSAHandle wsa;
  int x;
  int y;
} Animation;

typedef struct {
  TIMScript scripts[TIM_NUM_ANIMATIONS];

  TIMInterpreter interp;

  Animation anims[WSA_NUM_ANIMATIONS];

  uint8_t *frameBuffer;
  size_t frameBufferSize;
} TIMContext;

static TIMContext timCtx = {0};
static int isInit = 0;

static void WSAInit(TIMInterpreter *interp, uint16_t index, const char *wsaFile,
                    int x, int y, int offscreen, int flags);
static void WSARelease(TIMInterpreter *interp, int index);
static void WSADisplayFrame(TIMInterpreter *interp, int animIndex, int frame);
static void ShowDialogButtons(TIMInterpreter *interp, uint16_t functionId,
                              const uint16_t buttonStrIds[3]);
static void PlaySoundFX(TIMInterpreter *interp, uint16_t soundId);
static void PlayDialogue(TIMInterpreter *interp, uint16_t strId, int argc,
                         const uint16_t *argv);

void TIMInit(void) {
  printf("INIT TIM\n");
  memset(&timCtx, 0, sizeof(TIMContext));
  TIMInterpreterInit(&timCtx.interp);
  timCtx.interp.callbackCtx = &timCtx;
  timCtx.interp.callbacks = (TIMInterpreterCallbacks){
      .TIMInterpreterCallbacks_WSAInit = WSAInit,
      .TIMInterpreterCallbacks_WSADisplayFrame = WSADisplayFrame,
      .TIMInterpreterCallbacks_ShowDialogButtons = ShowDialogButtons,
      .TIMInterpreterCallbacks_WSARelease = WSARelease,
      .TIMInterpreterCallbacks_PlaySoundFX = PlaySoundFX,
      .TIMInterpreterCallbacks_PlayDialogue = PlayDialogue,
  };
}

void TIMLoad(uint16_t scriptId, const char *file) {
  if (!isInit) {
    TIMInit();
    isInit = 1;
  }

  printf("TIMLoad 0X%x %s\n", scriptId, file);
  TIMScript *script = timCtx.scripts + scriptId;
  assert(script);
  GameFile f = {0};
  assert(GameEnvironmentGetFileWithExt(&f, file, "TIM"));
  assert(TIMHandleFromBuffer(&script->tim, f.buffer, f.bufferSize));
}

static void doRenderWSAFrame(GameContext *gameCtx, const Animation *anim,
                             int frame) {
  assert(WSAHandleGetFrame(&anim->wsa, frame, timCtx.frameBuffer, frame != 0));
  DisplayRenderWSA(gameCtx->display, timCtx.frameBuffer, &anim->wsa, anim->x,
                   anim->y);
}

void TimLoadWSA(GameContext *gameCtx, uint16_t wsaIndex, const char *wsaFile,
                int x, int y, int offscreen, int flags) {

  printf(
      "TimLoadWSA wsaIndex=0X%X file='%s' x=%i y=%i offscreen=%i flags=0X%X\n",
      wsaIndex, wsaFile, x, y, offscreen, flags);

  Animation *anim = &timCtx.anims[wsaIndex];
  anim->x = x;
  anim->y = y;
  GameFile f = {0};
  assert(GameEnvironmentGetFileWithExt(&f, wsaFile, "WSA"));
  assert(WSAHandleFromBuffer(&anim->wsa, f.buffer, f.bufferSize));

  if (anim->wsa.header.palette == NULL) {
    anim->wsa.header.palette = GameContextGetDefaultPalette(gameCtx);
  }
  size_t fbSize = anim->wsa.header.width * anim->wsa.header.height;
  if (fbSize != timCtx.frameBufferSize && timCtx.frameBuffer != NULL) {
    free(timCtx.frameBuffer);
  }
  timCtx.frameBufferSize = fbSize;
  timCtx.frameBuffer = malloc(timCtx.frameBufferSize);
  memset(timCtx.frameBuffer, 0, timCtx.frameBufferSize);

  if (flags & 2) {
    printf("[WARNING] TimLoadWSA unhandled flag 2\n");
  }
  if (flags & 4) {
    // do we have a CPS file to show ?
    GameFile f = {0};
    if (GameEnvironmentGetFileWithExt(&f, wsaFile, "CPS")) {
      assert(0); // FIXME: to implement :)
    }
    doRenderWSAFrame(gameCtx, anim, 0);
  }
}

void TIMRun(GameContext *gameCtx, uint16_t scriptId, uint16_t loop) {
  printf("TIMRun 0X%x loop=%i\n", scriptId, loop);
  TIMScript *script = timCtx.scripts + scriptId;
  assert(script);
  assert(script->tim.avtl);
  TIMInterpreterStart(&timCtx.interp, &script->tim);
  timCtx.interp.callbackCtx = gameCtx;

  while (gameCtx->_shouldRun) {
    if (TIMInterpreterIsRunning(&timCtx.interp) == 0) {
      printf("TIM isRunning = 0\n");
      break;
    }
    printf("TIMInterpreterUpdate\n");
    TIMInterpreterUpdate(&timCtx.interp);
    SDL_Event e = {0};
    int mouse =
        DisplayWaitMouseEvent(gameCtx->display, &e, gameCtx->conf.tickLength);
    if (mouse == 0) {
      gameCtx->_shouldRun = 0;
      break;
    } else if (mouse == 1) {
      break;
    }
  }
}

void TIMRelease(uint16_t scriptId) {}

#pragma mark CALLBACKS

static void WSAInit(TIMInterpreter *interp, uint16_t wsaIndex,
                    const char *wsaFile, int x, int y, int offscreen,
                    int flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  TimLoadWSA(gameCtx, wsaIndex, wsaFile, x, y, offscreen, flags);
}

static void WSARelease(TIMInterpreter *interp, int wsaIndex) {}

static void WSADisplayFrame(TIMInterpreter *interp, int wsaIndex, int frame) {
  printf("TIM: callback WSADisplayFrame wsaIndex=%i frame=%i\n", wsaIndex,
         frame);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(timCtx.frameBuffer);
  Animation *anim = &timCtx.anims[wsaIndex];
  doRenderWSAFrame(gameCtx, anim, frame);
  DisplayRender(gameCtx->display);
}

static void ShowDialogButtons(TIMInterpreter *interp, uint16_t functionId,
                              const uint16_t buttonStrIds[3]) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  int buttonX = DIALOG_BUTTON1_X;
  for (int i = 0; i < 3; i++) {
    if (buttonStrIds[i] == 0XFFFF) {
      continue;
    }
    GameContextGetString(gameCtx, buttonStrIds[i], textBuffer,
                         TEXT_BUFFER_SIZE);
    printf("Button %i='%s'\n", i, textBuffer);
    if (i == 1) {
      buttonX = DIALOG_BUTTON2_X;
    } else if (i == 2) {
      buttonX = DIALOG_BUTTON3_X;
    }
    UIDrawTextButton(&gameCtx->display->defaultFont, gameCtx->display->pixBuf,
                     buttonX, DIALOG_BUTTON_Y, DIALOG_BUTTON_W, DIALOG_BUTTON_H,
                     textBuffer);
  }
}

static void PlaySoundFX(TIMInterpreter *interp, uint16_t soundId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextPlaySoundFX(gameCtx, soundId);
}

static void PlayDialogue(TIMInterpreter *interp, uint16_t stringId, int argc,
                         const uint16_t *argv) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextGetString(gameCtx, stringId, gameCtx->display->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  GameContextSetDialogF(gameCtx, stringId);
  GameContextPlayDialogSpeech(gameCtx, 0, stringId);
}
