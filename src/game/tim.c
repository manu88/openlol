#include "tim.h"
#include "SDL_events.h"
#include "audio.h"
#include "display.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_strings.h"
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

void TIMInit(void);

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
  // FIXME: This is Highly inefficient, but working for now :)
  memset(timCtx.frameBuffer, 0, timCtx.frameBufferSize);
  for (int i = 0; i <= frame; i++) {
    WSAHandleGetFrame(&anim->wsa, i, timCtx.frameBuffer, 1);
  }

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
    TIMInterpreterUpdate(&timCtx.interp);
    SDL_Event e = {0};
    int mouse = DisplayWaitMouseEvent(gameCtx->display, &e, 150);
    if (mouse == 0) {
      gameCtx->_shouldRun = 0;
      break;
    } else if (mouse == 1) {
      break;
    }
    DisplayUpdate(gameCtx->display);
  }
}

void TIMRelease(uint16_t scriptId) {}

#pragma mark CALLBACKS

static void callbackWSAInit(TIMInterpreter *interp, uint16_t wsaIndex,
                            const char *wsaFile, int x, int y, int offscreen,
                            int flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  TimLoadWSA(gameCtx, wsaIndex, wsaFile, x, y, offscreen, flags);
}

static void callbackWSARelease(TIMInterpreter *interp, int wsaIndex) {}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int wsaIndex,
                                    int frame) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(timCtx.frameBuffer);
  Animation *anim = &timCtx.anims[wsaIndex];
  doRenderWSAFrame(gameCtx, anim, frame);
}

static void callbackShowDialogButtons(TIMInterpreter *interp,
                                      uint16_t functionId,
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

static void callbackPlaySoundFX(TIMInterpreter *interp, uint16_t soundId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextPlaySoundFX(gameCtx, soundId);
}

static void callbackPlayDialogue(TIMInterpreter *interp, uint16_t stringId,
                                 int argc, const uint16_t *argv) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextGetString(gameCtx, stringId, gameCtx->display->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  GameContextSetDialogF(gameCtx, stringId);
  GameContextPlayDialogSpeech(gameCtx, 0, stringId);
}

static void callbackInitSceneDialog(TIMInterpreter *interp, int controlMode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextInitSceneDialog(gameCtx);
}

static void callbackFadeClearWindow(TIMInterpreter *interp, uint16_t param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  DisplayDoSceneFade(gameCtx->display, 10, gameCtx->conf.tickLength);
}

static int callbackContinueLoop(TIMInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;

  return AudioSystemGetCurrentVoiceIndex(&gameCtx->audio) != -1;
}

static void callbackSetLoop(TIMInterpreter *interp) {}

static void callbackCharChat(TIMInterpreter *interp, uint16_t charId,
                             uint16_t mode, uint16_t stringId) {
  printf("CharChat charId=%i mode=%i strId=%i\n", charId, mode, stringId);
  assert(0);
}

static void callbackRestoreAfterSceneDialog(TIMInterpreter *interp,
                                            int controlMode) {
  printf("RestoreAfterScene controlMode=%i\n", controlMode);
  assert(0);
}

static uint16_t callbackGiveItem(TIMInterpreter *interp, uint16_t param0,
                                 uint16_t param1, uint16_t param2) {
  printf("GiveItem param0=0X%X param1=0X%X param2=0X%X\n", param0, param1,
         param2);
  assert(0);
  return 0;
}

static void callbackCopyPage(TIMInterpreter *interp, uint16_t srcX,
                             uint16_t srcY, uint16_t destX, uint16_t destY,
                             uint16_t w, uint16_t h, uint16_t srcPage,
                             uint16_t destPage) {
  printf("CopyPage srcX=%i srcY=%i destX=%i destY=%i w=%i h=%i scrPage=%i "
         "destPage=%i\n",
         srcX, srcY, destX, destY, w, h, srcPage, destPage);
  assert(0);
}

static void callbackStopAllFunctions(TIMInterpreter *interp) {}

static void callbackClearTextField(TIMInterpreter *interp) {
  printf("ClearTextField\n");
  assert(0);
}

static void callbackLoadSoundFile(TIMInterpreter *interp, uint16_t soundId) {
  printf("LoadSoundFile soundId=%i\n", soundId);
  assert(0);
}

static void callbackPlayMusicTrack(TIMInterpreter *interp, uint16_t musicId) {
  printf("PlayMusicTrack soundId=%i\n", musicId);
  assert(0);
}

static void callbackUpdate(TIMInterpreter *interp) { printf("Update\n"); }

static void callbackSetPartyPos(TIMInterpreter *interp, uint16_t how,
                                uint16_t value) {
  printf("SetPartyPos how=0X%X value=%i\n", how, value);
  assert(0);
}

static void callbackDrawScene(TIMInterpreter *interp, uint16_t pageNum) {
  printf("DrawScene pageNum=%i\n", pageNum);
  assert(0);
}

static void callbackStartBackgroundAnimation(TIMInterpreter *interp,
                                             uint16_t animIndex,
                                             uint16_t part) {
  printf("StartBackgroundAnimation animIndex=%i part=%i\n", animIndex, part);
  assert(0);
}

void TIMInit(void) {
  memset(&timCtx, 0, sizeof(TIMContext));
  TIMInterpreterInit(&timCtx.interp);
  timCtx.interp.callbackCtx = &timCtx;
  timCtx.interp.callbacks = (TIMInterpreterCallbacks){
      .TIMInterpreterCallbacks_WSAInit = callbackWSAInit,
      .TIMInterpreterCallbacks_WSADisplayFrame = callbackWSADisplayFrame,
      .TIMInterpreterCallbacks_FadeClearWindow = callbackFadeClearWindow,
      .TIMInterpreterCallbacks_ShowDialogButtons = callbackShowDialogButtons,
      .TIMInterpreterCallbacks_WSARelease = callbackWSARelease,
      .TIMInterpreterCallbacks_PlaySoundFX = callbackPlaySoundFX,
      .TIMInterpreterCallbacks_PlayDialogue = callbackPlayDialogue,
      .TIMInterpreterCallbacks_InitSceneDialog = callbackInitSceneDialog,
      .TIMInterpreterCallbacks_ContinueLoop = callbackContinueLoop,
      .TIMInterpreterCallbacks_SetLoop = callbackSetLoop,
      .TIMInterpreterCallbacks_CharChat = callbackCharChat,
      .TIMInterpreterCallbacks_RestoreAfterSceneDialog =
          callbackRestoreAfterSceneDialog,
      .TIMInterpreterCallbacks_GiveItem = callbackGiveItem,
      .TIMInterpreterCallbacks_CopyPage = callbackCopyPage,
      .TIMInterpreterCallbacks_StopAllFunctions = callbackStopAllFunctions,
      .TIMInterpreterCallbacks_ClearTextField = callbackClearTextField,
      .TIMInterpreterCallbacks_LoadSoundFile = callbackLoadSoundFile,
      .TIMInterpreterCallbacks_PlayMusicTrack = callbackPlayMusicTrack,
      .TIMInterpreterCallbacks_Update = callbackUpdate,
      .TIMInterpreterCallbacks_SetPartyPos = callbackSetPartyPos,
      .TIMInterpreterCallbacks_DrawScene = callbackDrawScene,
      .TIMInterpreterCallbacks_StartBackgroundAnimation =
          callbackStartBackgroundAnimation,
  };
}
