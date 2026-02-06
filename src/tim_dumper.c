#include "tim_dumper.h"
#include "tim_interpreter.h"
#include <stdint.h>
#include <stdio.h>

static int inLoop = 0;

static void indent(void) {
  for (int i = 0; i < inLoop; i++) {
    printf("\t");
  }
}
static void callbackWSAInit(TIMInterpreter *interp, uint16_t index,
                            const char *wsaFile, int x, int y, int offscreen,
                            int flags) {
  indent();
  printf("WSAInit index=%i wsafile='%s' x=%i y=%i offscreen=%i flags=%i\n",
         index, wsaFile, x, y, offscreen, flags);
}

static void callbackWSARelease(TIMInterpreter *interp, int index) {
  indent();
  printf("WSARelease index=%i\n", index);
}

static void callbackWSADisplayFrame(TIMInterpreter *interp, int animIndex,
                                    int frame) {
  indent();
  printf("WSADisplayFrame index=%i frame=%i\n", animIndex, frame);
}

static void callbackPlayDialogue(TIMInterpreter *interp, uint16_t strId,
                                 int argc, const uint16_t *argv) {
  indent();
  printf("PlayDialogue strId=%i", strId);
  for (int i = 0; i < argc; i++) {
    printf(" arg%i=0X%X", i, argv[i]);
  }
  printf("\n");
}

static void callbackCharChat(TIMInterpreter *interp, uint16_t charId,
                             uint16_t mode, uint16_t stringId) {
  indent();
  printf("CharChat charId=%i mode=%i strId=%i\n", charId, mode, stringId);
}

static void callbackShowDialogButtons(TIMInterpreter *interp,
                                      uint16_t functionId,
                                      const uint16_t buttonStrIds[3]) {
  indent();
  printf("ShowDialogButtons functionId=%i btn0=0X%X btn1=0X%X btn2=0X%X\n",
         functionId, buttonStrIds[0], buttonStrIds[1], buttonStrIds[2]);
}

static void callbackInitSceneDialog(TIMInterpreter *interp, int controlMode) {
  indent();
  printf("InitSceneDialogs controlMode=%i\n", controlMode);
}

static void callbackRestoreAfterSceneDialog(TIMInterpreter *interp,
                                            int controlMode) {
  indent();
  printf("RestoreAfterScene controlMode=%i\n", controlMode);
}

static void callbackFadeClearWindow(TIMInterpreter *interp, uint16_t param) {
  indent();
  printf("FadeClearWindow param=%i\n", param);
}

static uint16_t callbackGiveItem(TIMInterpreter *interp, uint16_t param0,
                                 uint16_t param1, uint16_t param2) {
  indent();
  printf("GiveItem param0=0X%X param1=0X%X param2=0X%X\n", param0, param1,
         param2);
  return 0;
}
static void callbackCopyPage(TIMInterpreter *interp, uint16_t srcX,
                             uint16_t srcY, uint16_t destX, uint16_t destY,
                             uint16_t w, uint16_t h, uint16_t srcPage,
                             uint16_t destPage) {
  indent();
  printf("CopyPage srcX=%i srcY=%i destX=%i destY=%i w=%i h=%i scrPage=%i "
         "destPage=%i\n",
         srcX, srcY, destX, destY, w, h, srcPage, destPage);
}

static void callbackLoadSoundFile(TIMInterpreter *interp, uint16_t soundId) {
  indent();
  printf("LoadSoundFile soundId=%i\n", soundId);
}

static void callbackSetPartyPos(TIMInterpreter *interp, uint16_t how,
                                uint16_t value) {
  indent();
  printf("SetPartyPos how=0X%X value=%i\n", how, value);
}

static void callbackUpdate(TIMInterpreter *interp) {
  indent();
  printf("Update\n");
}

static void callbackPlayMusicTrack(TIMInterpreter *interp, uint16_t musicId) {
  indent();
  printf("PlayMusicTrack soundId=%i\n", musicId);
}

static void callbackPlaySoundFX(TIMInterpreter *interp, uint16_t soundId) {
  indent();
  printf("PlaySFX soundId=%i\n", soundId);
}

static void callbackStopAllFunctions(TIMInterpreter *interp) {
  indent();
  printf("StopAllFunctions\n");
}

static void callbackClearTextField(TIMInterpreter *interp) {
  indent();
  printf("ClearTextField\n");
}

static void callbackSetLoop(TIMInterpreter *interp) {
  indent();
  printf("SetLoopPoint\n");
  inLoop++;
}

static void callbackContinueLoop(TIMInterpreter *interp) {
  inLoop--;
  indent();
  printf("ContinueLoopPoint\n");
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
      .TIMInterpreterCallbacks_ClearTextField = callbackClearTextField,
      .TIMInterpreterCallbacks_LoadSoundFile = callbackLoadSoundFile,
      .TIMInterpreterCallbacks_PlayMusicTrack = callbackPlayMusicTrack,
      .TIMInterpreterCallbacks_Update = callbackUpdate,
      .TIMInterpreterCallbacks_SetPartyPos = callbackSetPartyPos,
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
