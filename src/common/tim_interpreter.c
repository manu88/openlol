#include "tim_interpreter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TIM_START_OFFSET 1

typedef enum {
  TIM_COMMAND_ID_STOP_ALL_FUNCS = 0X01,
  TIM_COMMAND_ID_WSA_INIT = 0X02,
  TIM_COMMAND_ID_WSA_RELEASE = 0X03,
  TIM_COMMAND_ID_WSA_DISPLAY_FRAME = 0X06,
  TIM_COMMAND_UNUSED_7 = 0X07,
  TIM_COMMAND_ID_CONTINUE_LOOP = 0X15,
  TIM_COMMAND_ID_RESET_ALL_RUNTIMES = 0X17,
  TIM_COMMAND_ID_CMD_RETURN_1 = 0X18,
  TIM_COMMAND_ID_EXEC_OPCODE = 0X19,
  TIM_COMMAND_ID_PROCESS_DIALOGUE = 0X1C,
  TIM_COMMAND_ID_DIALOG_BOX = 0X1D,
  TIM_COMMAND_SET_LOOP_IP = 0X14,
} TIM_COMMAND_ID;
#if 0
static const char *timCommandsName(uint8_t code) {
  switch ((TIM_COMMAND_ID)code) {
  case TIM_COMMAND_ID_STOP_ALL_FUNCS:
    return "TIM_COMMAND_ID_STOP_ALL_FUNCS";
  case TIM_COMMAND_ID_WSA_INIT:
    return "TIM_COMMAND_ID_WSA_INIT";
  case TIM_COMMAND_ID_WSA_RELEASE:
    return "TIM_COMMAND_ID_WSA_RELEASE";
  case TIM_COMMAND_ID_WSA_DISPLAY_FRAME:
    return "TIM_COMMAND_ID_WSA_DISPLAY_FRAME";
  case TIM_COMMAND_ID_CONTINUE_LOOP:
    return "TIM_COMMAND_ID_CONTINUE_LOOP";
  case TIM_COMMAND_ID_RESET_ALL_RUNTIMES:
    return "TIM_COMMAND_ID_RESET_ALL_RUNTIMES";
  case TIM_COMMAND_ID_CMD_RETURN_1:
    return "TIM_COMMAND_ID_CMD_RETURN_1";
  case TIM_COMMAND_ID_EXEC_OPCODE:
    return "TIM_COMMAND_ID_EXEC_OPCODE";
  case TIM_COMMAND_ID_PROCESS_DIALOGUE:
    return "TIM_COMMAND_ID_PROCESS_DIALOGUE";
  case TIM_COMMAND_ID_DIALOG_BOX:
    return "TIM_COMMAND_ID_DIALOG_BOX";
  case TIM_COMMAND_SET_LOOP_IP:
    return "TIM_COMMAND_SET_LOOP_IP";
    break;
  }
  assert(0);
  return NULL;
}
#endif
typedef enum {
  TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE = 0X00,
  TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE = 0X01,
  TIM_OPCODE_NOOP_2 = 0X02,
  TIM_OPCODE_GIVE_ITEM = 0X03,
  TIM_OPCODE_SET_PARTY_POS = 0X04,
  TIM_OPCODE_FADE_CLEAR_WINDOW = 0X05,
  TIM_OPCODE_COPY_REGION = 0X06,
  TIM_OPCODE_CHAR_CHAT = 0X07,
  TIM_OPCODE_DRAW_SCENE = 0X08,
  TIM_OPCODE_UPDATE = 0X09,
  TIM_OPCODE_CLEAR_TEXT_FIELD = 0X0A,
  TIM_OPCODE_LOAD_SOUND_FILE = 0X0B,
  TIM_OPCODE_PLAY_MUSIC_TRACK = 0X0C,
  TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT = 0X0D,
  TIM_OPCODE_PLAY_SOUND_FX = 0X0E,
  TIM_OPCODE_START_BACKGROUND_ANIM = 0X0F,
} TIM_OPCODE;

typedef struct {
  uint16_t len;
  uint16_t duration;
  uint8_t instrCode;
  uint8_t _unused;
} TImInstruction;

void TIMInterpreterInit(TIMInterpreter *interp) {
  memset(interp, 0, sizeof(TIMInterpreter));
  interp->loopStartPos = -1;
}

void TIMInterpreterStart(TIMInterpreter *interp, const TIMHandle *tim) {
  interp->loopStartPos = -1;
  interp->restartLoop = 0;
  interp->_tim = tim;
  interp->pos = TIM_START_OFFSET;
}

static void processOpCode(TIMInterpreter *interp, const uint16_t *params,
                          int numParams) {
  assert(numParams);
  numParams--;
  TIM_OPCODE timOpCode = params[0];
  params++;
  switch (timOpCode) {
  case TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE:
    assert(interp->callbacks.TIMInterpreterCallbacks_InitSceneDialog);
    interp->callbacks.TIMInterpreterCallbacks_InitSceneDialog(interp,
                                                              params[0]);
    return;
  case TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE:
    assert(interp->callbacks.TIMInterpreterCallbacks_RestoreAfterSceneDialog);
    interp->callbacks.TIMInterpreterCallbacks_RestoreAfterSceneDialog(
        interp, params[0]);
    return;
  case TIM_OPCODE_NOOP_2:
    return;
  case TIM_OPCODE_GIVE_ITEM:
    assert(interp->callbacks.TIMInterpreterCallbacks_GiveItem);
    interp->callbacks.TIMInterpreterCallbacks_GiveItem(interp, params[0],
                                                       params[1], params[2]);
    return;
  case TIM_OPCODE_SET_PARTY_POS:
    interp->callbacks.TIMInterpreterCallbacks_SetPartyPos(interp, params[0],
                                                          params[1]);
    return;
  case TIM_OPCODE_FADE_CLEAR_WINDOW:
    assert(interp->callbacks.TIMInterpreterCallbacks_FadeClearWindow);
    interp->callbacks.TIMInterpreterCallbacks_FadeClearWindow(interp,
                                                              params[0]);
    return;
  case TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT: {
    assert(interp->callbacks.TIMInterpreterCallbacks_PlayDialogue);
    interp->callbacks.TIMInterpreterCallbacks_PlayDialogue(
        interp, params[0], numParams - 1, numParams > 1 ? params + 1 : NULL);
    return;
  }
  case TIM_OPCODE_CLEAR_TEXT_FIELD:
    interp->callbacks.TIMInterpreterCallbacks_ClearTextField(interp);
    return;
  case TIM_OPCODE_LOAD_SOUND_FILE:
    interp->callbacks.TIMInterpreterCallbacks_LoadSoundFile(interp, params[0]);
    return;
  case TIM_OPCODE_PLAY_MUSIC_TRACK:
    interp->callbacks.TIMInterpreterCallbacks_PlayMusicTrack(interp, params[0]);
    return;
  case TIM_OPCODE_PLAY_SOUND_FX:
    interp->callbacks.TIMInterpreterCallbacks_PlaySoundFX(interp, params[0]);
    return;
  case TIM_OPCODE_COPY_REGION:
    interp->callbacks.TIMInterpreterCallbacks_CopyPage(
        interp, params[0], params[1], params[2], params[3], params[4],
        params[5], params[6], params[7]);
    return;
  case TIM_OPCODE_CHAR_CHAT:
    assert(interp->callbacks.TIMInterpreterCallbacks_CharChat);
    interp->callbacks.TIMInterpreterCallbacks_CharChat(interp, params[0],
                                                       params[1], params[2]);
    return;
  case TIM_OPCODE_DRAW_SCENE:
    interp->callbacks.TIMInterpreterCallbacks_DrawScene(interp, params[0]);
    return;
  case TIM_OPCODE_UPDATE:
    interp->callbacks.TIMInterpreterCallbacks_Update(interp);
    return;
  case TIM_OPCODE_START_BACKGROUND_ANIM:
    interp->callbacks.TIMInterpreterCallbacks_StartBackgroundAnimation(
        interp, params[0], params[1]);
    return;
  }
  printf("unhandled op code 0X%X %i params\n", timOpCode, numParams);
  assert(0);
}

static int processInstruction(TIMInterpreter *interp, uint16_t *buffer,
                              size_t pos) {
  const TImInstruction *instr = (const TImInstruction *)buffer;
  const uint16_t *instrParams = buffer + 3;
  int numParams =
      instr->len - 3; // 3 is size of minimum instruction size w/o params

  /*
  printf("0X%zX Instruction dur=0X%X len=%i code=%02X %s  %i params: ", pos,
         instr->duration, instr->len, instr->instrCode,
         timCommandsName(instr->instrCode), numParams);

  for (int i = 0; i < numParams; i++) {
    printf(" 0X%X ", instrParams[i]);
  }
  printf("\n");
  */
  interp->currentInstructionDuration = instr->duration;
  // printf("EXEC 0X%X\n", instr->instrCode);
  switch ((TIM_COMMAND_ID)instr->instrCode) {
  case TIM_COMMAND_ID_STOP_ALL_FUNCS:
    interp->callbacks.TIMInterpreterCallbacks_StopAllFunctions(interp);
    return instr->len;
  case TIM_COMMAND_ID_WSA_INIT: {
    uint16_t index = instrParams[0];
    uint16_t strParam = instrParams[1];
    uint16_t x = (int16_t)instrParams[2];
    uint16_t y = (int16_t)instrParams[3];
    uint16_t offscreen = instrParams[4];
    uint16_t wsaFlags = instrParams[5];
    const char *wsaFile = TIMHandleGetText(interp->_tim, strParam);
    interp->callbacks.TIMInterpreterCallbacks_WSAInit(interp, index, wsaFile, x,
                                                      y, offscreen, wsaFlags);
    return instr->len;
  }
  case TIM_COMMAND_ID_WSA_RELEASE:
    interp->callbacks.TIMInterpreterCallbacks_WSARelease(interp,
                                                         instrParams[0]);
    return instr->len;
  case TIM_COMMAND_ID_WSA_DISPLAY_FRAME: {
    int animIndex = instrParams[0];
    int frame = instrParams[1];
    interp->callbacks.TIMInterpreterCallbacks_WSADisplayFrame(interp, animIndex,
                                                              frame);
    return instr->len;
  }
  case TIM_COMMAND_ID_RESET_ALL_RUNTIMES:
    printf("UNIMPLEMENTED TIM_COMMAND_ID_RESET_ALL_RUNTIMES %i\n", numParams);
    return instr->len;
  case TIM_COMMAND_ID_CMD_RETURN_1:
    // printf("UNIMPLEMENTED TIM_COMMAND_ID_CMD_RETURN_1 %i\n", numParams);
    // FIXME: not sure, seems useless. Ignoring for now
    return instr->len;
  case TIM_COMMAND_ID_EXEC_OPCODE:
    processOpCode(interp, instrParams, numParams);
    return instr->len;
  case TIM_COMMAND_ID_PROCESS_DIALOGUE:
    printf("UNIMPLEMENTED TIM_COMMAND_ID_PROCESS_DIALOGUE\n");
    return instr->len;
  case TIM_COMMAND_ID_DIALOG_BOX: {
    uint16_t functionId = instrParams[0];
    interp->callbacks.TIMInterpreterCallbacks_ShowDialogButtons(
        interp, functionId, instrParams + 1);

    return instr->len;
  }
  case TIM_COMMAND_ID_CONTINUE_LOOP:
    if (interp->dontLoop == 0) {
      assert(interp->loopStartPos != -1);
      interp->restartLoop = 1;
#if 0
      if (interp->buttonState[0]) {
        printf("\t\t\tButton ok clicked\n");
        interp->buttonState[0] = 0;
        interp->restartLoop = 0;
      }
#endif
    }
    interp->restartLoop =
        interp->callbacks.TIMInterpreterCallbacks_ContinueLoop(interp);
    return instr->len;
  case TIM_COMMAND_SET_LOOP_IP:
    if (interp->dontLoop == 0) {
      interp->loopStartPos = pos;
    }
    interp->callbacks.TIMInterpreterCallbacks_SetLoop(interp);
    return instr->len;
  case TIM_COMMAND_UNUSED_7:
    return instr->len;
  }
  printf("unimplemented TIM OPCODE %X\n", instr->instrCode);
  assert(0);
  return instr->len;
}

int TIMInterpreterIsRunning(const TIMInterpreter *interp) {
  return interp->_tim && interp->pos < interp->_tim->avtlSize;
}

void TIMInterpreterButtonClicked(TIMInterpreter *interp, int buttonIndex) {
  assert(buttonIndex < 3);
  interp->buttonState[buttonIndex] = 1;
}

int TIMInterpreterUpdate(TIMInterpreter *interp) {
  int adv =
      processInstruction(interp, interp->_tim->avtl + interp->pos, interp->pos);

  if (interp->restartLoop) {
    interp->restartLoop = 0;
    interp->pos = interp->loopStartPos;
  } else {
    interp->pos += adv;
  }
  return interp->currentInstructionDuration;
}
