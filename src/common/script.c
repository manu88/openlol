#include "script.h"
#include "bytes.h"
#include "format_inf.h"
#include "script_builtins.h"
#include "script_disassembler.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int EMCStateSetOffset(EMCState *script, uint16_t offset) {
  script->ip = &script->dataPtr->data[offset];
  return 1;
}

int EMCStateStart(EMCState *script, int function) {
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
  functionOffset++;
  if (functionOffset >= (int)script->dataPtr->dataSize / 2) {
    printf("%X >= %X\n", functionOffset, (int)script->dataPtr->dataSize / 2);
    return 0;
  }
  return EMCStateSetOffset(script, functionOffset);
}

int EMCInterpreterIsValid(EMCInterpreter *interp, EMCState *script) {
  if (!script->ip || !script->dataPtr)
    return 0;
  return 1;
}

static void emitLineFunctionCall(EMCDisassembler *disasm, uint16_t funcCode) {

  if (funcCode >= getNumBuiltinFunctions()) {
    EMCDisassemblerEmitLine(disasm, "%s 0X%X", MNEMONIC_CALL, funcCode);
    return;
  }
  EMCDisassemblerEmitLine(disasm, "%s %s", MNEMONIC_CALL,
                          getBuiltinFunctions()[funcCode].name);
}

static void execOpCode(EMCInterpreter *interp, EMCState *script, int16_t opCode,
                       int16_t parameter) {

  switch ((ScriptCommand)opCode) {
  case OP_JUMP:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_JUMP,
                              parameter);
    } else {
      script->ip = script->dataPtr->data + parameter;
    }
    return;
  case OP_SETRETURNVALUE:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_SETRET,
                              parameter);
    } else {
      script->retValue = parameter;
    }
    return;
  case OP_PUSH_RETURN_OR_LOCATION:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_PUSH_RC,
                              parameter);
    } else {
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
    }
    return;
  case OP_PUSH:
  case OP_PUSH2:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_PUSH,
                              parameter);
    } else {
      script->stack[--script->sp] = parameter;
    }
    return;
  case OP_PUSH_VARIABLE:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_PUSH_VAR, parameter);
    } else {
      script->stack[--script->sp] = script->regs[parameter];
    }
    return;
  case OP_PUSH_LOCAL_VARIABLE:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_PUSH_LOC_VAR, parameter);
    } else {
      script->stack[--script->sp] =
          script->stack[(-(int32_t)(parameter + 2)) + script->bp];
    }
    return;
  case OP_PUSH_PARAMETER:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_PUSH_ARG, parameter);
    } else {
      script->stack[--script->sp] = script->stack[(parameter - 1) + script->bp];
    }
    return;
  case OP_POP_RETURN_OR_LOCATION:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_POP_RC,
                              parameter);
    } else {
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
    }
    return;
  case OP_POP_VARIABLE:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_POP,
                              parameter);
    } else {
      script->regs[parameter] = script->stack[script->sp++];
    }
    return;
  case OP_POP_LOCAL_VARIABLE:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_POP_LOC_VAR, parameter);
    } else {
      script->stack[(-(int32_t)(parameter + 2)) + script->bp] =
          script->stack[script->sp++];
    }
    return;
  case OP_POP_PARAMETER:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "POPPARAM %X", parameter);
    } else {
      script->stack[(parameter - 1) + script->bp] = script->stack[script->sp++];
    }
    return;
  case OP_STACK_REWIND:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_STACK_REWIND, parameter);
    } else {
      script->sp += parameter;
    }
    return;
  case OP_STACK_FORWARD:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X",
                              MNEMONIC_STACK_FORWARD, parameter);
    } else {
      script->sp -= parameter;
    }
    return;
  case OP_FUNCTION:
    if (interp->disassembler) {
      emitLineFunctionCall(interp->disassembler, parameter);
    } else {
      EMCInterpreterExecFunction(interp, script, (uint8_t)parameter);
    }
    return;
  case OP_JUMP_NE:
    parameter &= 0x7FFF;
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_JUMP_NE,
                              parameter);
    } else {
      if (!script->stack[script->sp++]) {
        script->ip = script->dataPtr->data + parameter;
      }
    }
    return;
  case OP_UNARY: {
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, "%s 0X%X", MNEMONIC_UNARY,
                              parameter);
    } else {
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
  }
    return;
  case OP_BINARY: {
    int16_t val1 = 0;
    int16_t val2 = 0;
    if (interp->disassembler == NULL) {
      val1 = script->stack[script->sp++];
      val2 = script->stack[script->sp++];
    }
    switch (parameter) {
    case BinaryOp_LogicalAND:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_LOGICAL_AND);
      } else {
        script->stack[--script->sp] = (val2 && val1) ? 1 : 0;
      }

      return;

    case BinaryOp_LogicalOR:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_LOGICAL_OR);
      } else {

        script->stack[--script->sp] = (val2 || val1) ? 1 : 0;
      }
      return;

    case BinaryOp_EQUAL:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_EQUAL);
      } else {
        script->stack[--script->sp] = (val1 == val2) ? 1 : 0;
      }
      return;

    case BinaryOp_NotEQUAL:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_NOT_EQUAL);
      } else {
        script->stack[--script->sp] = (val1 != val2) ? 1 : 0;
      }
      return;

    case BinaryOp_Inf:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_INF);
      } else {
        script->stack[--script->sp] = (val1 > val2) ? 1 : 0;
      }
      return;

    case BinaryOp_InfOrEq:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_INF_EQ);
      } else {
        script->stack[--script->sp] = (val1 >= val2) ? 1 : 0;
      }
      return;
    case BinaryOp_Greater:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_SUP);
      } else {
        script->stack[--script->sp] = (val1 < val2) ? 1 : 0;
      }
      return;

    case BinaryOp_GreaterOrEq:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_SUP_EQ);
      } else {
        script->stack[--script->sp] = (val1 <= val2) ? 1 : 0;
      }
      return;

    case BinaryOp_Add:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_ADD);
      } else {
        script->stack[--script->sp] = val1 + val2;
      }
      return;

    case BinaryOp_Minus:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_MINUS);
      } else {
        script->stack[--script->sp] = val2 - val1;
      }
      return;

    case BinaryOp_Multiply:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_MULTIPLY);
      } else {
        script->stack[--script->sp] = val1 * val2;
      }
      return;

    case BinaryOp_Divide:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_DIVIDE);
      } else {
        script->stack[--script->sp] = val2 / val1;
      }
      return;

    case BinaryOp_RShift:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_RIGHT_SHIFT);
      } else {
        script->stack[--script->sp] = val2 >> val1;
      }
      return;

    case BinaryOp_LShift:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_LEFT_SHIFT);
      } else {
        script->stack[--script->sp] = val2 << val1;
      }
      return;

    case BinaryOp_AND:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_AND);
      } else {
        script->stack[--script->sp] = val1 & val2;
      }
      return;

    case BinaryOp_OR:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_OR);
      } else {
        script->stack[--script->sp] = val1 | val2;
      }
      return;

    case BinaryOp_MOD:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_MOD);
      } else {
        script->stack[--script->sp] = val2 % val1;
      }
      return;

    case BinaryOp_XOR:
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, MNEMONIC_XOR);
      } else {
        script->stack[--script->sp] = val1 ^ val2;
      }
      return;

    default:
      printf("Unknown evaluate func: %d\n", parameter);
      assert(0);
    }
    return;
  }
  case OP_RETURN:
    if (interp->disassembler) {

    } else {
      if (script->sp >= kStackLastEntry) {
        script->ip = NULL;
        assert(0);
      } else {
        script->retValue = script->stack[script->sp++];
        uint16_t temp = script->stack[script->sp++];
        script->stack[kStackLastEntry] = 0;
        script->ip = &script->dataPtr->data[temp];
      }
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
