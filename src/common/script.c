#include "script.h"
#include "bytes.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ScriptVMInit(ScriptVM *vm) {
  memset(vm, 0, sizeof(ScriptVM));
  vm->stackPointer = STACK_SIZE;
  vm->framePointer = FRAME_POINTER_INIT;
}

void ScriptVMRelease(ScriptVM *vm) {
  if (vm->disassemble) {
    free(vm->disasmBuffer);
  }
}

void ScriptVMSetDisassembler(ScriptVM *vm) {
  if (vm->disassemble) {
    return;
  }
  vm->disassemble = 1;
  vm->disasmBufferSize = 256;
  vm->disasmBuffer = malloc(vm->disasmBufferSize);
  vm->disasmBufferIndex = 0;
}

void ScriptVMDump(const ScriptVM *vm) {
  printf("stack %i\n", vm->stackPointer);
  for (int i = 0; i < STACK_SIZE; i++) {
    printf("%i: %X\n", i, vm->stack[i]);
  }
}

typedef struct {
  uint16_t scriptStart;
  uint16_t currentAddress;
} ScriptContext;

void stackPush(ScriptVM *vm, uint16_t value) {
  if (vm->stackPointer == 0) {
    printf("stackPush: Stack Overflow %i\n", vm->stackPointer);
    assert(0);
    return;
  }
  vm->stack[--vm->stackPointer] = value;
}

uint16_t stackPop(ScriptVM *vm) {
  if (vm->stackPointer >= STACK_SIZE) {
    printf("stackPop: Stack Overflow %i\n", vm->stackPointer);
    assert(0);
    return 0;
  }

  return vm->stack[vm->stackPointer++];
}

uint16_t stackPeek(ScriptVM *vm, int position) {
  assert(position > 0);
  if (vm->stackPointer >= STACK_SIZE - position) {
    printf("stackPeek: Stack Overflow %i\n", vm->stackPointer);
    assert(0);
    return 0;
  }

  return vm->stack[vm->stackPointer + position - 1];
}

static void emitLine(ScriptVM *vm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t bSize = vm->disasmBufferSize - vm->disasmBufferIndex;
  if (bSize < 16) {
    vm->disasmBufferSize *= 2;
    vm->disasmBuffer = realloc(vm->disasmBuffer, vm->disasmBufferSize);
    bSize = vm->disasmBufferSize - vm->disasmBufferIndex;
  }
  size_t writtenSize =
      vsnprintf(vm->disasmBuffer + vm->disasmBufferIndex, bSize, fmt, args);
  if (writtenSize > bSize) {
    printf("no more size to write line, writtenSize=%zu, got bSize=%zu",
           writtenSize, bSize);
    assert(0);
  }
  vm->disasmBufferIndex += writtenSize;
  va_end(args);
}

static void parseInstruction(ScriptVM *vm, ScriptContext *ctx, uint8_t opCode,
                             uint16_t parameter) {
  switch ((ScriptCommand)opCode) {

  case OP_JUMP:
    if (vm->disassemble) {
      emitLine(vm, "JUMP 0X%X\n", parameter);
    } else {
      printf("JUMP %X (%X)\n", parameter, ctx->scriptStart + parameter);
    }
    break;
  case OP_SETRETURNVALUE:
    printf("SETRETURNVALUE %X\n", parameter);
    break;
  case OP_PUSH_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, "PushRC 0X%X\n", parameter);
    } else {
      printf("PUSH_RETURN_OR_LOCATION %X\n", parameter);
    }
    break;
  case OP_PUSH:
  case OP_PUSH2:
    if (vm->disassemble) {
      emitLine(vm, "PUSH 0X%X\n", parameter);
    } else {
      stackPush(vm, parameter);
    }
    break;
  case OP_PUSH_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, "PushVar 0X%X\n", parameter);
    } else {
      stackPush(vm, vm->variables[parameter]);
    }
    break;
  case OP_PUSH_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, "PushLocVar 0X%X\n", parameter);
    } else {
      printf("PUSH LOC VAR %X\n", parameter);
    }
    break;
  case OP_PUSH_PARAMETER:
    if (vm->disassemble) {
      emitLine(vm, "PushArg 0X%X\n", parameter);
    } else {
      stackPush(vm, vm->stack[vm->framePointer + parameter - 1]);
    }
    break;
  case OP_POP_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, "PopRC 0X%X\n", parameter);
    } else {
      if (parameter == 0) { // return value
        vm->returnValue = stackPop(vm);
      } else if (parameter == 1) { // POP FRAMEPOINTER + LOCATION
                                   //      stackPeek(vm, 2);
        //      vm->framePointer = (uint8_t)stackPop(vm);
        //      printf("OP_POP_RETURN_OR_LOCATION param 2 not implemented
        //      ;)\n"); assert(0);
      }
    }
    break;
  case OP_POP_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, "Pop 0X%X\n", parameter);
    } else {
      vm->variables[parameter] = stackPop(vm);
    }
    break;
  case OP_POP_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, "PopLocVar 0X%X\n", parameter);
    } else {
      printf("POP LOCAL VAR %X\n", parameter);
    }
    break;
  case OP_POP_PARAMETER:
    printf("POP PARAM %X\n", parameter);
    vm->stack[vm->framePointer + parameter - 1] = stackPop(vm);
    break;
  case OP_STACK_REWIND:
    if (vm->disassemble) {
      emitLine(vm, "StackRewind 0X%X\n", parameter);
    } else {

      vm->stackPointer += parameter;
    }
    break;
  case OP_STACK_FORWARD:
    if (vm->disassemble) {
      emitLine(vm, "StackForward 0X%X\n", parameter);
    } else {
      vm->stackPointer -= parameter;
    }
    break;
  case OP_FUNCTION:
    parameter &= 0xFF;
    if (vm->disassemble) {
      emitLine(vm, "call 0X%X\n", parameter);
    } else {
      printf("FUNCTION %X\n", parameter);
    }
    break;
  case OP_JUMP_NE:
    if (vm->disassemble) {
      emitLine(vm, "IfNotGo 0X%X\n", parameter);
    } else {
      printf("JUMP_NE %X\n", parameter);
    }
    break;
  case OP_UNARY:
    printf("UNARY %X\n", parameter);
    assert(parameter == 1 || parameter == 0 || parameter == 2);
    break;
  case OP_BINARY:
    assert(parameter <= 17);
    int16_t right = 0;
    int16_t left = 0;
    if (!vm->disassemble) {
      right = stackPop(vm);
      left = stackPop(vm);
    }

    switch (parameter) {
    case BinaryOp_LogicalAND:
      if (vm->disassemble) {
        emitLine(vm, "L_AND\n");
      } else {
        stackPush(vm, (left && right) ? 1 : 0);
      }
      break;
    case BinaryOp_LogicalOR:
      if (vm->disassemble) {
        emitLine(vm, "L_OR\n");
      } else {
        stackPush(vm, (left || right) ? 1 : 0);
      }
      break;
    case BinaryOp_EQUAL:
      if (vm->disassemble) {
        emitLine(vm, "EQ n");
      } else {
        stackPush(vm, (left == right) ? 1 : 0);
      }
      break;
    case BinaryOp_NotEQUAL:
      if (vm->disassemble) {
        emitLine(vm, "NEQ \n");
      } else {
        stackPush(vm, (left != right) ? 1 : 0);
      }
      break;
    case BinaryOp_Inf:
      if (vm->disassemble) {
        emitLine(vm, "INF\n");
      } else {
        stackPush(vm, (left < right) ? 1 : 0);
      }
      break;
    case BinaryOp_InfOrEq:
      if (vm->disassemble) {
        emitLine(vm, "IF_EQ \n");
      } else {
        stackPush(vm, (left <= right) ? 1 : 0);
      }
      break;
    case BinaryOp_Greater:
      if (vm->disassemble) {
        emitLine(vm, "SUP\n");
      } else {
        stackPush(vm, (left > right) ? 1 : 0);
      }
      break;
    case BinaryOp_GreaterOrEq:
      if (vm->disassemble) {
        emitLine(vm, "SUP_EQ\n");
      } else {
        stackPush(vm, (left >= right) ? 1 : 0);
      }
      break;
    case BinaryOp_Add:
      if (vm->disassemble) {
        emitLine(vm, "Add\n");
      } else {
        stackPush(vm, left + right);
      }
      break;
    case BinaryOp_Minus:
      if (vm->disassemble) {
        emitLine(vm, "Minus\n");
      } else {
        stackPush(vm, left - right);
      }
      break;
    case BinaryOp_Multiply:
      if (vm->disassemble) {
        emitLine(vm, "MULTIPLY\n");
      } else {
        stackPush(vm, left * right);
      }
      break;
    case BinaryOp_Divide:
      if (vm->disassemble) {
        emitLine(vm, "DIV\n");
      } else {
        stackPush(vm, left / right);
      }
      break;
    case BinaryOp_RShift:
      if (vm->disassemble) {
        emitLine(vm, "RSHIFT\n");
      } else {
        stackPush(vm, left >> right);
      }
      break;
    case BinaryOp_LShift:
      if (vm->disassemble) {
        emitLine(vm, "LSHIFT\n");
      } else {
        stackPush(vm, left << right);
      }
      break;
    case BinaryOp_AND:
      if (vm->disassemble) {
        emitLine(vm, "AND\n");
      } else {
        stackPush(vm, left & right);
      }
      break;
    case BinaryOp_OR:
      if (vm->disassemble) {
        emitLine(vm, "OR\n");
      } else {
        stackPush(vm, left | right);
      }
      break;
    case BinaryOp_MOD:
      if (vm->disassemble) {
        emitLine(vm, "MOD\n");
      } else {
        stackPush(vm, left % right);
      }
      break;
    case BinaryOp_XOR:
      if (vm->disassemble) {
        emitLine(vm, "XOR\n");
      } else {
        stackPush(vm, left ^ right);
      }
      break;
    }
    break;
  case OP_RETURN:
    if (vm->disassemble) {
      emitLine(vm, "RET\n");
    } else {
      printf("RET %X\n", parameter);
    }
    break;
  default:
    printf("0X04%X (%x) (%x)\n", ctx->currentAddress, opCode, parameter);
  }
}

int ScriptExec(ScriptVM *vm, const ScriptInfo *info) {
  uint32_t currentPos = 0;
  ScriptContext ctx = {0};
  ctx.scriptStart = currentPos;
  while (currentPos < info->scriptSize) {
    ctx.currentAddress = currentPos;
    uint16_t orig = *(info->scriptData + currentPos++);
    uint16_t current = swap_uint16(orig);
    uint16_t parameter = 0;
    uint8_t opcode = (current >> 8) & 0x1F;

    if ((current & 0x8000) != 0) {
      /* When this flag is set, the instruction is a GOTO with a 13bit address
       */
      opcode = 0;
      parameter = current & 0x7FFF;
    } else if ((current & 0x4000) != 0) {
      /* When this flag is set, the parameter is part of the instruction */
      parameter = (uint16_t)(int8_t)(current & 0xFF);
    } else if ((current & 0x2000) != 0) {
      /* When this flag is set, the parameter is in the next opcode */
      parameter = swap_uint16(*(info->scriptData + currentPos++));
    }
    parseInstruction(vm, &ctx, opcode, parameter);
  }

  return 0;
}
