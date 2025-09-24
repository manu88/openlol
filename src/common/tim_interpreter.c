#include "tim_interpreter.h"
#include "format_tim.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void TIMInterpreterInit(TIMInterpreter *interp) {
  memset(interp, 0, sizeof(TIMInterpreter));
  interp->procFunc = -1;
}

void TIMInterpreterRelease(TIMInterpreter *interp) {}

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

typedef enum {
  TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE = 0X00,
  TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE = 0X01,
  TIM_OPCODE_FADE_CLEAR_WINDOW = 0X05,
  TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT = 0X0D,
} TIM_OPCODE;

// FIXME duplicate in script_builtins.c
static void getLangString(uint16_t id) {
  if (id == 0xFFFF)
    return;

  uint16_t realId = id & 0x3FFF;
  int useLevelFile = 0;
  if (id & 0x4000) {
    useLevelFile = 0;
  } else {
    useLevelFile = 1;
  }
  printf("real lang string id=%i uselevel=%i\n", realId, useLevelFile);
}

static int execTIMOpCode(TIMInterpreter *interp, uint16_t opcode,
                         const uint16_t *param) {
  switch ((TIM_OPCODE)opcode) {
  case TIM_OPCODE_INIT_SCENE_WIN_DIALOGUE: {
    uint16_t controlMode = param[0];
    printf("OPCode init scene window dialogue p=0X%X\n", controlMode);
    return 1;
  }
  case TIM_OPCODE_PLAY_DIALOGUE_TALK_TEXT: {
    uint16_t strID = param[0];
    printf("OPCode play dialogue p=0X%X\n", strID);
    getLangString(strID);
    return 1;
  }
  case TIM_OPCODE_RESTORE_AFTER_SCENE_WIN_DIALOGUE: {
    uint16_t redraw = param[0];
    printf("OpCode restore after scene: redraw=%x\n", redraw);
    return 1;
  }
  case TIM_OPCODE_FADE_CLEAR_WINDOW: {
    return 1;
  }
  default:
    printf("unimplemented TIM opcode 0X%X\n", opcode);
    assert(0);
  }
  return -1;
}

static void advanceToOpCode(TIMInterpreter *interp, int opCode,
                            int prevProcFunc) {
  TimFunction *f = interp->_tim->functions + prevProcFunc;
  uint16_t len = f->ip[0];
  while ((f->ip[2] & 0xFF) != opCode) {
    if ((f->ip[2] & 0xFF) == 1) {
      f->ip[0] = len;
      break;
    }
    len = f->ip[0];
    f->ip += len;
  }

  // f->nextTime = _system->getMillis();
}

static int execCommand(TIMInterpreter *interp, uint8_t cmdID,
                       const uint16_t *param) {
  printf("execCommand %X\n", cmdID);
  switch ((TIM_COMMAND_ID)cmdID) {
  case TIM_COMMAND_ID_STOP_ALL_FUNCS: {
    printf("Stop all functions\n");
    return -1;
  }
  case TIM_COMMAND_ID_PROCESS_DIALOGUE: {
    printf("processDialogue\n");
    int res = 1;
    if (res == 0) {
      return 0;
    }

    interp->_tim->functions[interp->currentFunc].loopIp = 0;
    int prevProcFunc = interp->procFunc;
    interp->procFunc = -1;
    if (interp->procParam) {
      advanceToOpCode(interp, 21, prevProcFunc);
    }
    return res;
  }
  case TIM_COMMAND_ID_WSA_RELEASE: {
    int index = param[0];
    printf("WSA_RELEASE index=%i\n", index);
    return 1;
  }
  case TIM_COMMAND_ID_WSA_INIT: {
    int index = param[0];
    uint16_t strParam = param[1];
    uint16_t x = (int16_t)param[2];
    uint16_t y = (int16_t)param[3];
    uint16_t offscreen = param[4];
    uint16_t wsaFlags = param[5];
    const char *wsaFile = TIMHandleGetText(interp->_tim, index);
    printf("WSA_INIT index=%i strIndex=%X wsaFile='%s' x=%X y=%X offsreen=%X "
           "wsaFlags=%X\n",
           index, strParam, wsaFile, x, y, offscreen, wsaFlags);
    return 1;
  }
  case TIM_COMMAND_ID_WSA_DISPLAY_FRAME: {
    int frameIndex = param[0];
    int frame = param[1];
    printf("WSA_DISPLAY_FRAME frameIndex=%i frame=%X\n", frameIndex, frame);
    return 1;
  }
  case TIM_COMMAND_ID_EXEC_OPCODE: {
    const uint16_t opcode = *param++;
    printf("Exec Tim OPCode 0X%X\n", opcode);
    return execTIMOpCode(interp, opcode, param);
  }
  case TIM_COMMAND_ID_RESET_ALL_RUNTIMES: {
    return 1;
  }
  case TIM_COMMAND_ID_CMD_RETURN_1:
    return 1;
  case TIM_COMMAND_SET_LOOP_IP: {
    interp->_tim->functions[interp->currentFunc].loopIp =
        interp->_tim->functions[interp->currentFunc].ip;
    return 1;
  }
  case TIM_COMMAND_ID_DIALOG_BOX: {
    printf("DIALOG BOX\n");
    uint16_t func = *param;
    assert(func < TIM_NUM_FUNCTIONS);

    interp->procParam = func;
    // interp->_tim->clickedButton = 0;

    const char *tmpStr[3];

    for (int i = 1; i < 4; i++) {
      printf("%i %X\n", i, param[i]);
      if (param[i] != 0xFFFF) {
        getLangString(param[i]);
        // tmpStr[i-1] = getTableString(param[i]);

      } else {
        tmpStr[i - 1] = 0;
      }
    }
    return -3;
  }
  case TIM_COMMAND_ID_CONTINUE_LOOP: {
    return -2;
  }
  default:
    printf("unimplemented TIM commands %X\n", cmdID);
    assert(0);
  }
  return -1;
}

void TIMInterpreterStart(TIMInterpreter *interp, TIMHandle *tim) {
  interp->_tim = tim;
  if (interp->_tim->functions[0].ip == NULL) {
    interp->_tim->functions[0].ip = interp->_tim->functions[0].avtl;
  }
  interp->looped = 1;
}

int TIMInterpreterIsRunning(const TIMInterpreter *interp) {
  return interp->looped;
}

void TIMInterpreterUpdate(TIMInterpreter *interp) {
  if (interp->nextFunc == interp->currentFunc + 1) {
    interp->currentFunc++;
    if (interp->currentFunc >= TIM_NUM_FUNCTIONS) {
      interp->currentFunc = 0;
      interp->nextFunc = 0;
      return;
    }
  }

  printf(">>>ENTRER for (interp->currentFunc %i\n", interp->currentFunc);
  interp->cur = interp->_tim->functions + interp->currentFunc;

  interp->running = 1;

  while (interp->cur->ip && interp->running) {
    if (interp->procFunc != -1) {
      execCommand(interp, TIM_COMMAND_ID_PROCESS_DIALOGUE, &interp->procParam);
    }

    int8_t opcode = interp->cur->ip[2] & 0XFF;
    printf("opcode=%X from 0X%04X 0X%04X 0X%04X\n", opcode, interp->cur->ip[0],
           interp->cur->ip[1], interp->cur->ip[2]);

    int retCmd = execCommand(interp, opcode, interp->cur->ip + 3);
    switch (retCmd) {
    case 1:
      break;
    case -3:
      interp->procFunc = interp->currentFunc;
      //_currentTim->dlgFunc = -1;
      break;
    case -2:
      interp->running = 0;
      break;
    case -1:
      interp->looped = 0;
      interp->running = 0;
      break;
    default:
      assert(0);
    }
    if (interp->cur->ip) {
      interp->cur->ip += interp->cur->ip[0];
    }
  } // end while
  interp->nextFunc++;
}
