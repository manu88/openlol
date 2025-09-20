#include "script2.h"
#include "bytes.h"
#include "format_inf.h"
#include "script2_builtins.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// from https://github.com/OpenDUNE/OpenDUNE/blob/master/src/script/script.h

typedef enum {
  OP_JUMP = 0X00, // JUMP instruction given by the parameter.
  OP_SETRETURNVALUE =
      0X01, // Set the return value to the value given by the parameter.
  OP_PUSH_RETURN_OR_LOCATION =
      0X02,        // Push the return value (parameter = 0) or the location +
                   // framepointer (parameter = 1) on the stack.
  OP_PUSH = 0X03,  // Push a value given by the parameter on the stack.
  OP_PUSH2 = 0X04, // Identical to SCRIPT_PUSH.
  OP_PUSH_VARIABLE = 0X05,       // Push a variable on the stack.
  OP_PUSH_LOCAL_VARIABLE = 0X06, // Push a local variable on the stack.
  OP_PUSH_PARAMETER = 0X07,      // Push a parameter on the stack.
  OP_POP_RETURN_OR_LOCATION =
      0X08, // Pop the return value (parameter = 0) or the location +
            // framepointer (parameter = 1) from the stack.
  OP_POP_VARIABLE = 0X09,       // Pop a variable from the stack.
  OP_POP_LOCAL_VARIABLE = 0X0A, // Pop a local variable from the stack.
  OP_POP_PARAMETER = 0X0B,      // Pop a parameter from the stack.
  OP_STACK_REWIND =
      0X0C, // Add a value given by the parameter to the stackpointer.
  OP_STACK_FORWARD =
      0X0D, // Substract a value given by the parameter to the stackpointer.
  OP_FUNCTION = 0X0E, // Execute a function by its ID given by the parameter.
  OP_JUMP_NE = 0X0F,  // Jump to the instruction given by the parameter if the
                      // last entry on the stack is non-zero.
  OP_UNARY = 0X10,    // Perform unary operations.
  OP_BINARY = 0X11,   // Perform binary operations.
  OP_RETURN = 0X12    // Return from a subroutine.
} ScriptCommand;

int EMCInterpreterLoad(EMCInterpreter *interp, const INFScript *infScript,
                       EMCData *data) {
  data->text = infScript->chunks[kText]._data;
  data->ordr = (uint16_t *)infScript->chunks[kEmc2Ordr]._data;
  data->ordrSize = infScript->chunks[kEmc2Ordr]._size / 2;
  data->data = (uint16_t *)infScript->chunks[kData]._data;
  data->dataSize = infScript->chunks[kData]._size;
  return 0;
}

void EMCInterpreterUnload(EMCInterpreter *interp, EMCData *data) {}

void EMCStateInit(EMCState *scriptState, const EMCData *data) {
  memset(scriptState, 0, sizeof(EMCState));
  scriptState->dataPtr = data;
  scriptState->ip = NULL;
  scriptState->stack[kStackLastEntry] = 0;
  scriptState->bp = kStackSize + 1;
  scriptState->sp = kStackLastEntry;
}

int EMCInterpreterStart(EMCInterpreter *interp, EMCState *script,
                        int function) {
  assert(script->dataPtr);
  assert(function >= 0);
  if (function >= (int)script->dataPtr->ordrSize) {
    printf("Function %i >= %i\n", function, (int)script->dataPtr->ordrSize);
    return 0;
  }

  uint16_t functionOffset = swap_uint16(script->dataPtr->ordr[function]);
  printf("function %i -- functionOffset=0X%X\n", function, functionOffset);
  if (functionOffset == 0xFFFF) {
    printf("no such function\n");
    return 0;
  }

  if (functionOffset + 1 >= (int)script->dataPtr->dataSize / 2) {
    printf("%X >= %X\n", functionOffset + 1,
           (int)script->dataPtr->dataSize / 2);
    return 0;
  }

  script->ip = &script->dataPtr->data[functionOffset + 1];
  return 1;
}

int EMCInterpreterIsValid(EMCInterpreter *interp, EMCState *script) {
  if (!script->ip || !script->dataPtr)
    return 0;
  return 1;
}

static void execOpCode(EMCInterpreter *interp, EMCState *script, int16_t opCode,
                       int16_t parameter) {

  switch ((ScriptCommand)opCode) {

  case OP_JUMP:
    script->ip = script->dataPtr->data + parameter;
    return;
  case OP_SETRETURNVALUE:
    script->retValue = parameter;
    return;
  case OP_PUSH_RETURN_OR_LOCATION:
    switch (parameter) {
    case 0:
      script->stack[--script->sp] = script->retValue;
      return;

    case 1:
      script->stack[--script->sp] = script->ip - script->dataPtr->data + 1;
      script->stack[--script->sp] = script->bp;
      script->bp = script->sp + 2;
      return;

    default:
      assert(0);
      script->ip = NULL;
    }
    return;
  case OP_PUSH:
  case OP_PUSH2:
    script->stack[--script->sp] = parameter;
    return;
  case OP_PUSH_VARIABLE:
    script->stack[--script->sp] = script->regs[parameter];
    return;
  case OP_PUSH_LOCAL_VARIABLE:
    script->stack[--script->sp] =
        script->stack[(-(int32_t)(parameter + 2)) + script->bp];
    return;
  case OP_PUSH_PARAMETER:
    script->stack[--script->sp] = script->stack[(parameter - 1) + script->bp];
    return;
  case OP_POP_RETURN_OR_LOCATION:
    switch (parameter) {
    case 0:
      script->retValue = script->stack[script->sp++];
      break;

    case 1:
      if (script->sp >= kStackLastEntry) {
        // assert(0);
        script->ip = NULL;
      } else {
        script->bp = script->stack[script->sp++];
        script->ip = script->dataPtr->data + script->stack[script->sp++];
      }
      break;

    default:
      script->ip = NULL;
    }
    return;
  case OP_POP_VARIABLE:
    script->regs[parameter] = script->stack[script->sp++];
    return;
  case OP_POP_LOCAL_VARIABLE:
    script->stack[(-(int32_t)(parameter + 2)) + script->bp] =
        script->stack[script->sp++];
    return;
  case OP_POP_PARAMETER:
    script->stack[(parameter - 1) + script->bp] = script->stack[script->sp++];
    return;
  case OP_STACK_REWIND:
    script->sp += parameter;
    return;
  case OP_STACK_FORWARD:
    script->sp -= parameter;
    return;
  case OP_FUNCTION:
    EMCInterpreterExecFunction(interp, script, (uint8_t)parameter);
    return;
  case OP_JUMP_NE:
    if (!script->stack[script->sp++]) {
      parameter &= 0x7FFF;
      script->ip = script->dataPtr->data + parameter;
    }
    return;
  case OP_UNARY: {
    int16_t value = script->stack[script->sp];
    switch (parameter) {
    case 0:
      if (!value)
        script->stack[script->sp] = 1;
      else
        script->stack[script->sp] = 0;
      break;

    case 1:
      script->stack[script->sp] = -value;
      break;

    case 2:
      script->stack[script->sp] = ~value;
      break;

    default:
      printf("Unknown negation func: %d\n", parameter);
      script->ip = NULL;
      assert(0);
    }
  }
    return;
  case OP_BINARY: {
    int16_t ret = 0;
    uint8_t error = 0;

    int16_t val1 = script->stack[script->sp++];
    int16_t val2 = script->stack[script->sp++];

    switch (parameter) {
    case 0:
      ret = (val2 && val1) ? 1 : 0;
      break;

    case 1:
      ret = (val2 || val1) ? 1 : 0;
      break;

    case 2:
      ret = (val1 == val2) ? 1 : 0;
      break;

    case 3:
      ret = (val1 != val2) ? 1 : 0;
      break;

    case 4:
      ret = (val1 > val2) ? 1 : 0;
      break;

    case 5:
      ret = (val1 >= val2) ? 1 : 0;
      break;

    case 6:
      ret = (val1 < val2) ? 1 : 0;
      break;

    case 7:
      ret = (val1 <= val2) ? 1 : 0;
      break;

    case 8:
      ret = val1 + val2;
      break;

    case 9:
      ret = val2 - val1;
      break;

    case 10:
      ret = val1 * val2;
      break;

    case 11:
      ret = val2 / val1;
      break;

    case 12:
      ret = val2 >> val1;
      break;

    case 13:
      ret = val2 << val1;
      break;

    case 14:
      ret = val1 & val2;
      break;

    case 15:
      ret = val1 | val2;
      break;

    case 16:
      ret = val2 % val1;
      break;

    case 17:
      ret = val1 ^ val2;
      break;

    default:
      printf("Unknown evaluate func: %d\n", parameter);
      error = 1;
    }
    if (error) {
      assert(0);
      script->ip = NULL;
    }

    else {
      script->stack[--script->sp] = ret;
    }
  }
    return;
  case OP_RETURN:
    if (script->sp >= kStackLastEntry) {
      script->ip = NULL;
      assert(0);
    } else {
      script->retValue = script->stack[script->sp++];
      uint16_t temp = script->stack[script->sp++];
      script->stack[kStackLastEntry] = 0;
      script->ip = &script->dataPtr->data[temp];
    }
    return;
  }
}

int EMCInterpreterRun(EMCInterpreter *interp, EMCState *script) {
  if (!script->ip) {
    return 0;
  }

  const uint32_t instOffset =
      (uint32_t)((const uint8_t *)script->ip -
                 (const uint8_t *)script->dataPtr->data);
  if ((int32_t)instOffset < 0 || instOffset >= script->dataPtr->dataSize) {
    printf("Attempt to execute out of bounds: 0x%.08X out of 0x%.08X\n",
           instOffset, script->dataPtr->dataSize);
    assert(0);
  }
  int16_t code = swap_uint16(*script->ip++);
  int16_t opcode = (code >> 8) & 0x1F;

  int16_t parameter = 0;
  if (code & 0x8000) {
    opcode = 0;
    parameter = code & 0x7FFF;
  } else if (code & 0x4000) {
    parameter = (int8_t)(code);
  } else if (code & 0x2000) {
    parameter = swap_uint16(*script->ip++);
  }

  if (opcode > 18) {
    printf("Unknown script opcode: %d at offset 0x%.08X\n", opcode, instOffset);
  } else {
    // printf("%X Exec opcode=%X param=%X\n", instOffset,
    // opcode,interp->_parameter);
    execOpCode(interp, script, opcode, parameter);
    // debugC(5, kDebugLevelScript, "[0x%.08X] EMCInterpreter::%s([%d/%u])",
    // instOffset, _opcodes[opcode].desc, _parameter, (uint)_parameter);
    //(this->*(_opcodes[opcode].proc))(script);
  }
  return script->ip != NULL;
}

int script2Main(int argc, char *argv[]) {
  printf("Script2 Main\n");
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *iffData = readBinaryFile("LEVEL1.INF", &fileSize, &readSize);
  printf("Read %zi bytes\n", readSize);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, readSize)) {
    printf("INFScriptFromBuffer error\n");
    return 1;
  }

  EMCInterpreter interp = {0};
  EMCData dat = {0};
  EMCInterpreterLoad(&interp, &script, &dat);
  printf("ordrsize=%u datasize=%u\n", dat.ordrSize, dat.dataSize);
  EMCState state = {0};
  EMCStateInit(&state, &dat);
#if 0
  for (int i = 0; i < dat.ordrSize; i++) {
    if (dat.ordr[i] != 0XFFFF) {
      printf("%X %X\n", i, swap_uint16(dat.ordr[i]));
    }
  }
#endif
  int functionId = atoi(argv[0]);
  if (!EMCInterpreterStart(&interp, &state, functionId)) {
    printf("EMCInterpreterStart: invalid\n");
  }
  int n = 0;

  state.regs[0] = -1; // flags
  state.regs[1] = -1; // charnum
  state.regs[2] = 0;  // item
  state.regs[3] = 0;
  state.regs[4] = 0;
  state.regs[5] = functionId;

  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
  }
  printf("Exec'ed %i instructions\n", n);
  INFScriptRelease(&script);
  return 0;
}
