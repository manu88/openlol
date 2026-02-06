#include "tim_dumper.h"
#include "tim_interpreter.h"
#include <stdio.h>

static int inLoop = 0;

static void indent(void) {
  if (inLoop) {
    printf("\t");
  }
}
void callbackWSAInit(TIMInterpreter *interp, uint16_t index,
                     const char *wsaFile, int x, int y, int offscreen,
                     int flags) {
  indent();
  printf("WSA init index=%i wsafile='%s' x=%i y=%i offscreen=%i flags=%i\n",
         index, wsaFile, x, y, offscreen, flags);
}

void callbackWSARelease(TIMInterpreter *interp, int index) {
  indent();
  printf("WSA release index=%i\n", index);
}

void callbackWSADisplayFrame(TIMInterpreter *interp, int animIndex, int frame) {
  indent();
  printf("WSA display frame index=%i frame=%i\n", animIndex, frame);
}

void callbackPlayDialogue(TIMInterpreter *interp, uint16_t strId, int argc,
                          const uint16_t *argv) {
  indent();
  printf("Play dialogue strId=%i args=[", strId);
  for (int i = 0; i < argc; i++) {
    if (i > 0) {
      printf(", ");
    }
    printf("0X%X ", argv[i]);
  }
  printf("]\n");
}

void callbackCharChat(TIMInterpreter *interp, uint16_t charId, uint16_t mode,
                      uint16_t stringId) {
  indent();
  printf("Char chat charId=%i mode=%i strId=%i\n", charId, mode, stringId);
}

void callbackShowDialogButtons(TIMInterpreter *interp, uint16_t functionId,
                               const uint16_t buttonStrIds[3]) {
  indent();
  printf("Show dialog buttons functionId=%i btn0=0X%X btn1=0X%X btn2=0X%X\n",
         functionId, buttonStrIds[0], buttonStrIds[1], buttonStrIds[2]);
}

void callbackInitSceneDialog(TIMInterpreter *interp, int controlMode) {
  indent();
  printf("Init Scene dialogs controlMode=%i\n", controlMode);
}

void callbackRestoreAfterSceneDialog(TIMInterpreter *interp, int controlMode) {
  indent();
  printf("Restore after scene controlMode=%i\n", controlMode);
}

void callbackFadeClearWindow(TIMInterpreter *interp, uint16_t param) {
  indent();
  printf("FadeClear window param=%i\n", param);
}

uint16_t callbackGiveItem(TIMInterpreter *interp, uint16_t param0,
                          uint16_t param1, uint16_t param2) {
  indent();
  printf("Give Item param0=0X%X param1=0X%X param2=0X%X\n", param0, param1,
         param2);
  return 0;
}
void callbackCopyPage(TIMInterpreter *interp, uint16_t srcX, uint16_t srcY,
                      uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                      uint16_t srcPage, uint16_t destPage) {
  indent();
  printf("CopyPage srcX=%i srcY=%i destX=%i destY=%i w=%i h=%i scrPage=%i "
         "destPage=%i\n",
         srcX, srcY, destX, destY, w, h, srcPage, destPage);
}

void callbackPlaySoundFX(TIMInterpreter *interp, uint16_t soundId) {
  indent();
  printf("Play SFX soundId=%i\n", soundId);
}

void callbackStopAllFunctions(TIMInterpreter *interp) {
  indent();
  printf("Stop all functions\n");
}

void callbackSetLoop(TIMInterpreter *interp) {
  inLoop = 1;
  printf("Set Loop point\n");
}

void callbackContinueLoop(TIMInterpreter *interp) {
  inLoop = 0;
  printf("Continue Loop point\n");
}

void DumpTim(const TIMHandle *handle) {
  TIMInterpreter interp;
  TIMInterpreterInit(&interp);
  TIMInterpreterStart(&interp, handle);

  interp.callbacks = (TIMInterpreterCallbacks){
      .TIMInterpreterCallbacks_WSAInit = callbackWSAInit,
      .TIMInterpreterCallbacks_WSARelease = callbackWSARelease,
      .TIMInterpreterCallbacks_WSADisplayFrame = callbackWSADisplayFrame,
      .TIMInterpreterCallbacks_PlayDialogue = callbackPlayDialogue,
      .TIMInterpreterCallbacks_CharChat = callbackCharChat,
      .TIMInterpreterCallbacks_ShowDialogButtons = callbackShowDialogButtons,
      .TIMInterpreterCallbacks_InitSceneDialog = callbackInitSceneDialog,
      .TIMInterpreterCallbacks_RestoreAfterSceneDialog =
          callbackRestoreAfterSceneDialog,
      .TIMInterpreterCallbacks_FadeClearWindow = callbackFadeClearWindow,
      .TIMInterpreterCallbacks_GiveItem = callbackGiveItem,
      .TIMInterpreterCallbacks_CopyPage = callbackCopyPage,
      .TIMInterpreterCallbacks_PlaySoundFX = callbackPlaySoundFX,
      .TIMInterpreterCallbacks_StopAllFunctions = callbackStopAllFunctions,
  };

  interp.debugCallbacks = (TIMInterpreterDebugCallbacks){
      .TIMInterpreterDebugCallbacks_SetLoop = callbackSetLoop,
      .TIMInterpreterDebugCallbacks_ContinueLoop = callbackContinueLoop,
  };

  interp.dontLoop = 1;
  while (TIMInterpreterIsRunning(&interp)) {
    TIMInterpreterUpdate(&interp);
  }
}
