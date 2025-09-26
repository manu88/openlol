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
  TIM_COMMAND_ID_CONTINUE_LOOP = 0X15,
  TIM_COMMAND_ID_RESET_ALL_RUNTIMES = 0X17,
  TIM_COMMAND_ID_CMD_RETURN_1 = 0X18,
  TIM_COMMAND_ID_EXEC_OPCODE = 0X19,
  TIM_COMMAND_ID_PROCESS_DIALOGUE = 0X1C,
  TIM_COMMAND_ID_DIALOG_BOX = 0X1D,
  TIM_COMMAND_SET_LOOP_IP = 0X14,
} TIM_COMMAND_ID;

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

typedef enum {
  TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE = 0X00,
  TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE = 0X01,
  TIM_OPCODE_FADE_CLEAR_WINDOW = 0X05,
  TIM_OPCODE_CLEAR_TEXT_FIELD = 0X0A,
  TIM_OPCODE_LOAD_SOUND_FILE = 0X0B,
  TIM_OPCODE_PLAY_MUSIC_TRACK = 0X0C,
  TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT = 0X0D,
  TIM_OPCODE_PLAY_SOUND_FX = 0X0E,
} TIM_OPCODE;

typedef struct {
  uint16_t len;
  uint16_t something0;
  uint8_t instrCode;
  uint8_t something1;
} TImInstruction;

void TIMInterpreterInit(TIMInterpreter *interp) {
  memset(interp, 0, sizeof(TIMInterpreter));
  interp->loopStartPos = -1;
}
void TIMInterpreterRelease(TIMInterpreter *interp) {}

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim,
                         uint32_t timeMs) {
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
    printf("\t TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE %i params\n", numParams);
    break;
  case TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE:
    printf("\t TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE %i params\n",
           numParams);
    break;
  case TIM_OPCODE_FADE_CLEAR_WINDOW:
    printf("\t TIM_OPCODE_FADE_CLEAR_WINDOW %i params\n", numParams);
    break;
  case TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT: {
    const uint16_t stringId = params[0];
    printf("\t TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT %i params\n", numParams);
    if (interp->callbacks.TIMInterpreterCallbacks_PlayDialogue) {
      interp->callbacks.TIMInterpreterCallbacks_PlayDialogue(interp, stringId);
    }
    break;
  }
  case TIM_OPCODE_CLEAR_TEXT_FIELD:
    printf("\t TIM_OPCODE_CLEAR_TEXT_FIELD %i params\n", numParams);
    break;
  case TIM_OPCODE_LOAD_SOUND_FILE:
    printf("\t TIM_OPCODE_LOAD_SOUND_FILE %i params\n", numParams);
    break;
  case TIM_OPCODE_PLAY_MUSIC_TRACK:
    printf("\t TIM_OPCODE_PLAY_MUSIC_TRACK %i params\n", numParams);
    break;
  case TIM_OPCODE_PLAY_SOUND_FX:
    printf("\t TIM_OPCODE_PLAY_SOUND_FX %i params\n", numParams);
    break;
  default:
    printf("Unimplemented TIM Opcode %X\n", params[0]);
    assert(0);
  }
}

static int processInstruction(TIMInterpreter *interp, uint16_t *buffer,
                              size_t pos) {
  const TImInstruction *instr = (const TImInstruction *)buffer;
  const uint16_t *instrParams = buffer + 3;
  int numParams =
      instr->len - 3; // 3 is size of minimum instruction size w/o params

  printf("0X%zX Instruction len=%i code=%02X %s  %i params: ", pos, instr->len,
         instr->instrCode, timCommandsName(instr->instrCode), numParams);
  for (int i = 0; i < numParams; i++) {
    printf(" 0X%X ", instrParams[i]);
  }
  printf("\n");

  switch ((TIM_COMMAND_ID)instr->instrCode) {

  case TIM_COMMAND_ID_STOP_ALL_FUNCS:

    break;
  case TIM_COMMAND_ID_WSA_INIT: {
    // int index = instrParams[0];
    uint16_t strParam = instrParams[1];
    uint16_t x = (int16_t)instrParams[2];
    uint16_t y = (int16_t)instrParams[3];
    uint16_t offscreen = instrParams[4];
    uint16_t wsaFlags = instrParams[5];
    const char *wsaFile = TIMHandleGetText(interp->_tim, strParam);
    if (interp->callbacks.TIMInterpreterCallbacks_WSAInit) {
      interp->callbacks.TIMInterpreterCallbacks_WSAInit(interp, wsaFile, x, y,
                                                        offscreen, wsaFlags);
    }
    break;
  }
  case TIM_COMMAND_ID_WSA_RELEASE:

    break;
  case TIM_COMMAND_ID_WSA_DISPLAY_FRAME: {
    int frameIndex = instrParams[0];
    int frame = instrParams[1];
    if (interp->callbacks.TIMInterpreterCallbacks_WSADisplayFrame) {
      interp->callbacks.TIMInterpreterCallbacks_WSADisplayFrame(
          interp, frameIndex, frame);
    }
    break;
  }

  case TIM_COMMAND_ID_CONTINUE_LOOP:
    assert(interp->loopStartPos != -1);
    interp->restartLoop = 1;
    break;
  case TIM_COMMAND_ID_RESET_ALL_RUNTIMES:

    break;
  case TIM_COMMAND_ID_CMD_RETURN_1:

    break;
  case TIM_COMMAND_ID_EXEC_OPCODE:

    break;
  case TIM_COMMAND_ID_PROCESS_DIALOGUE:

    break;
  case TIM_COMMAND_ID_DIALOG_BOX:

    break;
  case TIM_COMMAND_SET_LOOP_IP:
    interp->loopStartPos = pos;
    break;
  default:
    assert(0);
  }
  if (instr->instrCode == TIM_COMMAND_ID_EXEC_OPCODE) {
    processOpCode(interp, instrParams, numParams);
  }
  return instr->len;
}

int TIMInterpreterIsRunning(const TIMInterpreter *interp) {
  return interp->pos < interp->_tim->avtlSize;
}
void TIMInterpreterUpdate(TIMInterpreter *interp, uint32_t timeMs) {
  int adv =
      processInstruction(interp, interp->_tim->avtl + interp->pos, interp->pos);
  if (interp->restartLoop) {
    interp->restartLoop = 0;
    interp->pos = interp->loopStartPos;
  } else {
    interp->pos += adv;
  }
}
