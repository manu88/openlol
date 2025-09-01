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
  size_t writtenSize =
      vsnprintf(vm->disasmBuffer + vm->disasmBufferIndex, bSize, fmt, args);
  if (writtenSize > bSize) {
    printf("no more size to write line, writtenSize=%zu, got bSize=%zu",
           writtenSize, bSize);
    assert(0);
  }
  printf("Wrote %zu bytes\n", writtenSize);
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
    printf("PUSH_RETURN_OR_LOCATION %X\n", parameter);
    assert(parameter == 1 || parameter == 0);
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
    printf("PUSH VAR %X\n", parameter);
    stackPush(vm, vm->variables[parameter]);
    break;
  case OP_PUSH_LOCAL_VARIABLE:
    printf("PUSH LOC VAR %X\n", parameter);
    break;
  case OP_PUSH_PARAMETER:
    printf("PUSH PARAM %X\n", parameter);
    stackPush(vm, vm->stack[vm->framePointer + parameter - 1]);
    break;
  case OP_POP_RETURN_OR_LOCATION:
    printf("POP RET OR LOC %X\n", parameter);
    if (parameter == 0) { // return value
      vm->returnValue = stackPop(vm);
    } else if (parameter == 1) { // POP FRAMEPOINTER + LOCATION
                                 //      stackPeek(vm, 2);
      //      vm->framePointer = (uint8_t)stackPop(vm);
      //      printf("OP_POP_RETURN_OR_LOCATION param 2 not implemented ;)\n");
      //      assert(0);
    }
    assert(parameter == 1 || parameter == 0);
    break;
  case OP_POP_VARIABLE:
    printf("POP VAR %X\n", parameter);
    vm->variables[parameter] = stackPop(vm);
    break;
  case OP_POP_LOCAL_VARIABLE:
    printf("POP LOCAL VAR %X\n", parameter);
    break;
  case OP_POP_PARAMETER:
    printf("POP PARAM %X\n", parameter);
    vm->stack[vm->framePointer + parameter - 1] = stackPop(vm);
    break;
  case OP_STACK_REWIND:
    printf("STA REWIND %X\n", parameter);
    vm->stackPointer += parameter;
    break;
  case OP_STACK_FORWARD:
    printf("STA FORWARD %X\n", parameter);
    vm->stackPointer -= parameter;
    break;
  case OP_FUNCTION:
    parameter &= 0xFF;
    printf("FUNCTION %X\n", parameter);
    break;
  case OP_JUMP_NE:
    printf("JUMP_NE %X\n", parameter);
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
      stackPush(vm, (left && right) ? 1 : 0);
      break;
    case BinaryOp_LogicalOR:
      stackPush(vm, (left || right) ? 1 : 0);
      break;
    case BinaryOp_EQUAL:
      stackPush(vm, (left == right) ? 1 : 0);
      break;
    case BinaryOp_NotEQUAL:
      stackPush(vm, (left != right) ? 1 : 0);
      break;
    case BinaryOp_Inf:
      stackPush(vm, (left < right) ? 1 : 0);
      break;
    case BinaryOp_InfOrEq:
      stackPush(vm, (left <= right) ? 1 : 0);
      break;
    case BinaryOp_Greater:
      stackPush(vm, (left > right) ? 1 : 0);
      break;
    case BinaryOp_GreaterOrEq:
      stackPush(vm, (left >= right) ? 1 : 0);
      break;
    case BinaryOp_Add:
      if (vm->disassemble) {
        emitLine(vm, "MULTIPLY 0X%X\n", parameter);
      } else {
        stackPush(vm, left + right);
      }
      break;
    case BinaryOp_Minus:
      stackPush(vm, left - right);
      break;
    case BinaryOp_Multiply:
      stackPush(vm, left * right);
      break;
    case BinaryOp_Divide:
      stackPush(vm, left / right);
      break;
    case BinaryOp_RShift:
      stackPush(vm, left >> right);
      break;
    case BinaryOp_LShift:
      stackPush(vm, left << right);
      break;
    case BinaryOp_AND:
      stackPush(vm, left & right);
      break;
    case BinaryOp_OR:
      stackPush(vm, left | right);
      break;
    case BinaryOp_MOD:
      stackPush(vm, left % right);
      break;
    case BinaryOp_XOR:
      stackPush(vm, left ^ right);
      break;
    }
    break;
  case OP_RETURN:
    printf("RET %X\n", parameter);
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
