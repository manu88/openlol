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
  vm->disasmBufferSize = 512;
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
  uint16_t currentWorld;
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

static void emitLine(ScriptVM *vm, ScriptContext *ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t bSize = vm->disasmBufferSize - vm->disasmBufferIndex;
  if (bSize < 64) {
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

  if (vm->showDisamComment) {
    bSize = vm->disasmBufferSize - writtenSize;
    writtenSize = snprintf(vm->disasmBuffer + vm->disasmBufferIndex, bSize,
                           "; 0X%04X\n", ctx->currentAddress + vm->addrOffset);

    if (writtenSize > bSize) {
      printf("no more size to write line, writtenSize=%zu, got bSize=%zu\n",
             writtenSize, bSize);
      assert(0);
    }
    vm->disasmBufferIndex += writtenSize;
  } else {
    writtenSize =
        snprintf(vm->disasmBuffer + vm->disasmBufferIndex, bSize, "\n");

    vm->disasmBufferIndex += writtenSize;
  }
}

static int parseInstruction(ScriptVM *vm, ScriptContext *ctx, uint8_t opCode,
                            uint16_t parameter) {
  switch ((ScriptCommand)opCode) {

  case OP_JUMP:
    if (vm->disassemble) {
      emitLine(vm, ctx, "JUMP 0X%X", parameter);
    } else {
      printf("JUMP %X (%X)\n", parameter, ctx->scriptStart + parameter);
    }
    return 1;
  case OP_SETRETURNVALUE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "SetRET 0X%X", parameter);
    } else {
      printf("SETRETURNVALUE %X\n", parameter);
    }
    return 1;
  case OP_PUSH_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, ctx, "PushRC 0X%X", parameter);
    } else {
      printf("PUSH_RETURN_OR_LOCATION %X", parameter);
    }
    return 1;
  case OP_PUSH:
  case OP_PUSH2:
    if (vm->disassemble) {
      emitLine(vm, ctx, "PUSH 0X%X", parameter);
    } else {
      stackPush(vm, parameter);
    }
    return 1;
  case OP_PUSH_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "PushVar 0X%X", parameter);
    } else {
      stackPush(vm, vm->variables[parameter]);
    }
    return 1;
  case OP_PUSH_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "PushLocVar 0X%X", parameter);
    } else {
      printf("PUSH LOC VAR %X", parameter);
    }
    return 1;
  case OP_PUSH_PARAMETER:
    if (vm->disassemble) {
      emitLine(vm, ctx, "PushArg 0X%X", parameter);
    } else {
      printf("PushArg 0X%X\n", parameter);
      stackPush(vm, vm->stack[vm->framePointer + parameter - 1]);
    }
    return 1;
  case OP_POP_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, ctx, "PopRC 0X%X", parameter);
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
    return 1;
  case OP_POP_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "Pop 0X%X", parameter);
    } else {
      vm->variables[parameter] = stackPop(vm);
    }
    return 1;
  case OP_POP_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "PopLocVar 0X%X", parameter);
    } else {
      printf("POP LOCAL VAR %X\n", parameter);
    }
    return 1;
  case OP_POP_PARAMETER:
    if (vm->disassemble) {
      emitLine(vm, ctx, "POPPARAM %X", parameter);
    } else {
      printf("POP PARAM %X\n", parameter);
      vm->stack[vm->framePointer + parameter - 1] = stackPop(vm);
    }
    return 1;
  case OP_STACK_REWIND:
    if (vm->disassemble) {
      emitLine(vm, ctx, "StackRewind 0X%X", parameter);
    } else {
      vm->stackPointer += parameter;
    }
    return 1;
  case OP_STACK_FORWARD:
    if (vm->disassemble) {
      emitLine(vm, ctx, "StackForward 0X%X", parameter);
    } else {
      vm->stackPointer -= parameter;
    }
    return 1;
  case OP_FUNCTION:
    if (vm->disassemble) {
      emitLine(vm, ctx, "call 0X%X", parameter);
    } else {
      parameter &= 0xFF;
      printf("FUNCTION %X\n", parameter);
    }
    return 1;
  case OP_JUMP_NE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "IfNotGo 0X%X", parameter);
    } else {
      printf("JUMP_NE %X\n", parameter);
    }
    return 1;
  case OP_UNARY:

    if (vm->disassemble) {
      emitLine(vm, ctx, "Unary 0X%X", parameter);
    } else {
      printf("UNARY %X\n", parameter);
      assert(parameter == 1 || parameter == 0 || parameter == 2);
    }

    return 1;
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
        emitLine(vm, ctx, "L_AND");
      } else {
        stackPush(vm, (left && right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_LogicalOR:
      if (vm->disassemble) {
        emitLine(vm, ctx, "L_OR");
      } else {
        stackPush(vm, (left || right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_EQUAL:
      if (vm->disassemble) {
        emitLine(vm, ctx, "EQ");
      } else {
        stackPush(vm, (left == right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_NotEQUAL:
      if (vm->disassemble) {
        emitLine(vm, ctx, "NEQ");
      } else {
        stackPush(vm, (left != right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Inf:
      if (vm->disassemble) {
        emitLine(vm, ctx, "INF");
      } else {
        stackPush(vm, (left < right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_InfOrEq:
      if (vm->disassemble) {
        emitLine(vm, ctx, "IF_EQ");
      } else {
        stackPush(vm, (left <= right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Greater:
      if (vm->disassemble) {
        emitLine(vm, ctx, "SUP");
      } else {
        stackPush(vm, (left > right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_GreaterOrEq:
      if (vm->disassemble) {
        emitLine(vm, ctx, "SUP_EQ");
      } else {
        stackPush(vm, (left >= right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Add:
      if (vm->disassemble) {
        emitLine(vm, ctx, "Add");
      } else {
        stackPush(vm, left + right);
      }
      return 1;
    case BinaryOp_Minus:
      if (vm->disassemble) {
        emitLine(vm, ctx, "Minus");
      } else {
        stackPush(vm, left - right);
      }
      return 1;
    case BinaryOp_Multiply:
      if (vm->disassemble) {
        emitLine(vm, ctx, "MULTIPLY");
      } else {
        stackPush(vm, left * right);
      }
      return 1;
    case BinaryOp_Divide:
      if (vm->disassemble) {
        emitLine(vm, ctx, "DIV");
      } else {
        stackPush(vm, left / right);
      }
      return 1;
    case BinaryOp_RShift:
      if (vm->disassemble) {
        emitLine(vm, ctx, "RSHIFT");
      } else {
        stackPush(vm, left >> right);
      }
      return 1;
    case BinaryOp_LShift:
      if (vm->disassemble) {
        emitLine(vm, ctx, "LSHIFT");
      } else {
        stackPush(vm, left << right);
      }
      return 1;
    case BinaryOp_AND:
      if (vm->disassemble) {
        emitLine(vm, ctx, "AND");
      } else {
        stackPush(vm, left & right);
      }
      return 1;
    case BinaryOp_OR:
      if (vm->disassemble) {
        emitLine(vm, ctx, "OR");
      } else {
        stackPush(vm, left | right);
      }
      return 1;
    case BinaryOp_MOD:
      if (vm->disassemble) {
        emitLine(vm, ctx, "MOD");
      } else {
        stackPush(vm, left % right);
      }
      return 1;
    case BinaryOp_XOR:
      if (vm->disassemble) {
        emitLine(vm, ctx, "XOR");
      } else {
        stackPush(vm, left ^ right);
      }
      return 1;
    default:
      printf("unknown binary op %X\n", parameter);
      return 0;
    }
    break;
  case OP_RETURN:
    if (vm->disassemble) {
      emitLine(vm, ctx, "RET");
    } else {
      printf("RET %X\n", parameter);
    }
    return 1;
  default:
    printf("unknown instruction at 0X04%X opcode=%x (param=%x)\n",
           ctx->currentAddress, opCode, parameter);
  }
  return 0;
}

int ScriptExec(ScriptVM *vm, const ScriptInfo *info) {
  uint32_t currentPos = 0;
  ScriptContext ctx = {0};
  ctx.scriptStart = currentPos;
  while (currentPos < info->scriptSize) {
    ctx.currentAddress = currentPos;
    uint16_t orig = *(info->scriptData + currentPos++);
    ctx.currentWorld = orig;
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
    if (!parseInstruction(vm, &ctx, opcode, parameter)) {
      return 0;
    }
  }

  return 1;
}
