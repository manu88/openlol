#include "script.h"
#include "bytes.h"
#include "formats/format_inf.h"
#include "logger.h"
#include "script_builtins.h"
#include "script_disassembler.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *EMCStateGetDataString(const EMCState *state, int16_t index) {
  return INFScriptGetDataString(state->dataPtr, index);
}

static void emitLineFunctionCall(EMCDisassembler *disasm, uint16_t funcCode,
                                 uint32_t instOffset) {

  if (funcCode >= getNumBuiltinFunctions()) {
    EMCDisassemblerEmitLine(disasm, instOffset, "%s 0X%X", MNEMONIC_CALL,
                            funcCode);
    printf("func %X is outside builtins (size=%zx) at offset 0X%X\n", funcCode,
           getNumBuiltinFunctions(), instOffset);
    return;
  }
  assert(getBuiltinFunctions()[funcCode].name);
  EMCDisassemblerEmitLine(disasm, instOffset, "%s %s", MNEMONIC_CALL,
                          getBuiltinFunctions()[funcCode].name);
}

static void StackPush(EMCState *s, uint16_t val) { s->stack[--s->sp] = val; }
static uint16_t StackPop(EMCState *s) { return s->stack[s->sp++]; }

static void execOpCode(EMCInterpreter *interp, EMCState *script,
                       uint16_t opCode, uint16_t parameter,
                       uint32_t instOffset) {
  if (interp->disassembler) {
    int offsetIndex = INFScriptIsOffset(script->dataPtr, instOffset + 1);
    if (offsetIndex != -1) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s %i",
                              MNEMONIC_LABEL_OFFSET, offsetIndex);
    }
  }

  switch ((ScriptCommand)opCode) {
  case OP_JUMP:
    Log("SCRIPT", "JUMP 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_JUMP, parameter);
    } else {
      script->ip = script->dataPtr->scriptData + parameter;
#if 0
      printf("After jump ip=%lX\n", script->ip - script->dataPtr->data);
      if (script->ip - script->dataPtr->data == 0) {
        printf("STOP \n");
        script->ip = NULL;
      }
#endif
    }
    return;
  case OP_SETRETURNVALUE:
    Log("SCRIPT", "SETRET 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_SETRET, parameter);
    } else {
      script->retValue = parameter;
    }
    return;
  case OP_PUSH_RETURN_OR_LOCATION:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_PUSH_RC, parameter);
    } else {
      switch (parameter) {
      case 0:
        Log("SCRIPT", "PUSHRET");
        StackPush(script, script->retValue);
        return;
      case 1:
        Log("SCRIPT", "PUSHRETLOC");
        StackPush(script, script->ip - script->dataPtr->scriptData + 1);
        StackPush(script, script->bp);
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
    Log("SCRIPT", "PUSH 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_PUSH, parameter);
    } else {
      StackPush(script, parameter);
    }
    return;
  case OP_PUSH_VARIABLE:
    Log("SCRIPT", "PUSHVAR 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_PUSH_VAR, parameter);
    } else {
      StackPush(script, script->regs[parameter]);
    }
    return;
  case OP_PUSH_LOCAL_VARIABLE:
    Log("SCRIPT", "PUSHLOCVAR 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_PUSH_LOC_VAR, parameter);
    } else {
      StackPush(script, script->stack[script->bp - parameter - 2]);
    }
    return;
  case OP_PUSH_PARAMETER:
    Log("SCRIPT", "PUSHPARAM 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_PUSH_ARG, parameter);
    } else {
      uint16_t val = script->stack[script->bp + parameter - 1];
      StackPush(script, val);
    }
    return;
  case OP_POP_RETURN_OR_LOCATION:
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_POP_RC, parameter);
    } else {
      switch (parameter) {
      case 0:
        Log("SCRIPT", "POPRET");
        script->retValue = StackPop(script);
        break;

      case 1:
        Log("SCRIPT", "POPLOC");
        if (script->sp >= STACK_LAST_ENTRY) {
          // assert(0);
          script->ip = NULL;
        } else {
          script->bp = StackPop(script);
          script->ip = script->dataPtr->scriptData + StackPop(script);
        }
        break;
      default:
        script->ip = NULL;
      }
    }
    return;
  case OP_POP_VARIABLE:
    Log("SCRIPT", "POPVAR 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_POP, parameter);
    } else {
      script->regs[parameter] = StackPop(script);
    }
    return;
  case OP_POP_LOCAL_VARIABLE:
    Log("SCRIPT", "POPLOCVAR 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_POP_LOC_VAR, parameter);
    } else {
      script->stack[script->bp - parameter - 2] = StackPop(script);
    }
    return;
  case OP_POP_PARAMETER:
    Log("SCRIPT", "POPPARAM 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "POPPARAM %X",
                              parameter);
    } else {
      script->stack[script->bp + parameter - 1] = StackPop(script);
    }
    return;
  case OP_STACK_REWIND:
    Log("SCRIPT", "OP_STACK_REWIND 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_STACK_REWIND, parameter);
    } else {
      script->sp += parameter;
    }
    return;
  case OP_STACK_FORWARD:
    Log("SCRIPT", "OP_STACK_FORWARD 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_STACK_FORWARD, parameter);
    } else {
      script->sp -= parameter;
    }
    return;
  case OP_FUNCTION:
    if (interp->disassembler) {
      emitLineFunctionCall(interp->disassembler, parameter, instOffset);
    } else {
      EMCInterpreterExecFunction(interp, script, (uint8_t)parameter);
    }
    return;
  case OP_JUMP_NE:
    parameter &= 0x7FFF;
    Log("SCRIPT", "JUMP_NE 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_JUMP_NE, parameter);
    } else {
      if (!StackPop(script)) {
        script->ip = script->dataPtr->scriptData + parameter;
      }
    }
    return;
  case OP_UNARY: {
    Log("SCRIPT", "UNARY 0X%X", parameter);
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, "%s 0X%X",
                              MNEMONIC_UNARY, parameter);
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
      val1 = StackPop(script);
      val2 = StackPop(script);
    }
    switch (parameter) {
    case BinaryOp_LogicalAND:
      Log("SCRIPT", "LogicalAND 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_LOGICAL_AND);
      } else {
        StackPush(script, (val2 && val1) ? 1 : 0);
      }

      return;

    case BinaryOp_LogicalOR:
      Log("SCRIPT", "LogicalOR 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_LOGICAL_OR);
      } else {
        StackPush(script, (val2 || val1) ? 1 : 0);
      }
      return;

    case BinaryOp_EQUAL:
      Log("SCRIPT", "EQUAL 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_EQUAL);
      } else {
        StackPush(script, (val1 == val2) ? 1 : 0);
      }
      return;

    case BinaryOp_NotEQUAL:
      Log("SCRIPT", "NOT EQUAL 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_NOT_EQUAL);
      } else {
        StackPush(script, (val1 != val2) ? 1 : 0);
      }
      return;

    case BinaryOp_Inf:
      Log("SCRIPT", "INF 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_INF);
      } else {
        StackPush(script, (val1 > val2) ? 1 : 0);
      }
      return;

    case BinaryOp_InfOrEq:
      Log("SCRIPT", "INF_OR_EQ 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_INF_EQ);
      } else {
        StackPush(script, (val1 >= val2) ? 1 : 0);
      }
      return;
    case BinaryOp_Greater:
      Log("SCRIPT", "GREATER 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_SUP);
      } else {
        StackPush(script, (val1 < val2) ? 1 : 0);
      }
      return;

    case BinaryOp_GreaterOrEq:
      Log("SCRIPT", "GREATER_OR_EQ 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_SUP_EQ);
      } else {
        StackPush(script, (val1 <= val2) ? 1 : 0);
      }
      return;

    case BinaryOp_Add:
      Log("SCRIPT", "ADD 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_ADD);
      } else {
        StackPush(script, val1 + val2);
      }
      return;

    case BinaryOp_Minus:
      Log("SCRIPT", "MINUS 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_MINUS);
      } else {
        StackPush(script, val2 - val1);
      }
      return;

    case BinaryOp_Multiply:
      Log("SCRIPT", "MULT 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_MULTIPLY);
      } else {
        StackPush(script, val1 * val2);
      }
      return;

    case BinaryOp_Divide:
      Log("SCRIPT", "DIV 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_DIVIDE);
      } else {
        StackPush(script, val2 / val1);
      }
      return;

    case BinaryOp_RShift:
      Log("SCRIPT", "RShift 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_RIGHT_SHIFT);
      } else {
        StackPush(script, val2 >> val1);
      }
      return;

    case BinaryOp_LShift:
      Log("SCRIPT", "LShift 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset,
                                MNEMONIC_LEFT_SHIFT);
      } else {
        StackPush(script, val2 << val1);
      }
      return;

    case BinaryOp_AND:
      Log("SCRIPT", "AND 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_AND);
      } else {
        StackPush(script, val1 & val2);
      }
      return;

    case BinaryOp_OR:
      Log("SCRIPT", "OR 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_OR);
      } else {
        StackPush(script, val1 | val2);
      }
      return;

    case BinaryOp_MOD:
      Log("SCRIPT", "MOD 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_MOD);
      } else {
        StackPush(script, val1 % val2);
      }
      return;

    case BinaryOp_XOR:
      Log("SCRIPT", "XOR 0X%X 0X%X", val1, val2);
      if (interp->disassembler) {
        EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_XOR);
      } else {
        StackPush(script, val1 ^ val2);
      }
      return;

    default:
      printf("Unknown evaluate func: %d\n", parameter);
      assert(0);
    }
    return;
  }
  case OP_RETURN:
    Log("SCRIPT", "RET");
    if (interp->disassembler) {
      EMCDisassemblerEmitLine(interp->disassembler, instOffset, MNEMONIC_RET);
    } else {
      if (script->sp >= STACK_LAST_ENTRY) {
        script->ip = NULL;
        assert(0);
      } else {
        script->retValue = StackPop(script);
        uint16_t temp = StackPop(script);
        script->stack[STACK_LAST_ENTRY] = 0;
        script->ip = &script->dataPtr->scriptData[temp];
      }
    }
    return;
  case OP_LABEL_OFFSET:
    assert(0); // Implement me :)
  }
}

void EMCStateInit(EMCState *scriptState, const INFScript *script) {
  memset(scriptState, 0, sizeof(EMCState));
  scriptState->dataPtr = script;
  scriptState->ip = NULL;
  scriptState->stack[STACK_LAST_ENTRY] = 0;
  scriptState->bp = STACK_SIZE + 1;
  scriptState->sp = STACK_LAST_ENTRY;
  EMCStateSetOffset(scriptState, 0);
}

int EMCStateSetOffset(EMCState *script, uint16_t offset) {
  script->ip = script->dataPtr->scriptData + offset;
  return 1;
}

int EMCStateStart(EMCState *state, int function) {
  assert(state->dataPtr);
  assert(function >= 0);
  if (function >= (int)INFScriptGetNumFunctions(state->dataPtr)) {
    printf("Function %i >= %i\n", function,
           INFScriptGetNumFunctions(state->dataPtr));
    return 0;
  }

  uint16_t functionOffset =
      INFScriptGetFunctionOffset(state->dataPtr, function);
  if (functionOffset == 0xFFFF) {
    return 0;
  }
  functionOffset++;
  if (functionOffset >= (int)state->dataPtr->scriptDataSize / 2) {
    printf("%X >= %X\n", functionOffset,
           (int)state->dataPtr->scriptDataSize / 2);
    return 0;
  }
  return EMCStateSetOffset(state, functionOffset);
}

int EMCInterpreterIsValid(EMCInterpreter *interp, EMCState *state) {
  if (!state->ip || !state->dataPtr)
    return 0;
  return 1;
}

int EMCInterpreterRun(EMCInterpreter *interp, EMCState *state) {
  if (!state->ip) {
    return 0;
  }

  const uint32_t instOffset =
      (ptrdiff_t)(state->ip - state->dataPtr->scriptData);

  if ((int32_t)instOffset < 0 ||
      instOffset >= state->dataPtr->scriptDataSize / 2) {
    printf("Attempt to execute out of bounds: 0x%.08X out of 0x%.08X\n",
           instOffset, state->dataPtr->scriptDataSize);
    state->ip = NULL;
    return 0;
  }
  uint16_t code = swap_uint16(*state->ip++);
  uint16_t opcode = (code >> 8) & 0x1F;

  uint16_t parameter = 0;
  if (code & 0x8000) {
    opcode = 0;
    parameter = code & 0x7FFF;
  } else if (code & 0x4000) {
    parameter = (int8_t)(code);
  } else if (code & 0x2000) {
    parameter = swap_uint16(*state->ip++);
  }

  if (opcode > 18) {
    printf("Unknown script opcode: %d at offset 0x%.08X\n", opcode, instOffset);
  } else {
    execOpCode(interp, state, opcode, parameter, instOffset);
  }
  return state->ip != NULL;
}
