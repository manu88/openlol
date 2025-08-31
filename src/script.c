#include "script.h"
#include "bytes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void ScriptVMInit(ScriptVM *vm) {
  memset(vm, 0, sizeof(ScriptVM));
  vm->stackPointer = STACK_SIZE;
  vm->framePointer = FRAME_POINTER_INIT;
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

static void parseInstruction(ScriptVM *vm, ScriptContext *ctx, uint8_t opCode,
                             uint16_t parameter) {

  printf("0X%X: (%i)", ctx->currentAddress, vm->stackPointer);
  switch ((ScriptCommand)opCode) {

  case OP_JUMP:
    printf("JUMP %X (%X)\n", parameter, ctx->scriptStart + parameter);
    break;
  case OP_SETRETURNVALUE:
    printf("SETRETURNVALUE %X\n", parameter);
    break;
  case OP_PUSH_RETURN_OR_LOCATION:
    printf("PUSH_RETURN_OR_LOCATION %X\n", parameter);
    assert(parameter == 1 || parameter == 0);
    break;
  case OP_PUSH:
    printf("PUSH2 %04X\n", parameter);
    stackPush(vm, parameter);
    break;
  case OP_PUSH2:
    printf("PUSH %04X\n", parameter);
    stackPush(vm, parameter);
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
    printf("BINARY %X\n", parameter);
    assert(parameter <= 17);
    int16_t right = stackPop(vm);
    int16_t left = stackPop(vm);

    switch (parameter) {
    case 0:
      stackPush(vm, (left && right) ? 1 : 0);
      break; /* left && right */
    case 1:
      stackPush(vm, (left || right) ? 1 : 0);
      break; /* left || right */
    case 2:
      stackPush(vm, (left == right) ? 1 : 0);
      break; /* left == right */
    case 3:
      stackPush(vm, (left != right) ? 1 : 0);
      break; /* left != right */
    case 4:
      stackPush(vm, (left < right) ? 1 : 0);
      break; /* left <  right */
    case 5:
      stackPush(vm, (left <= right) ? 1 : 0);
      break; /* left <= right */
    case 6:
      stackPush(vm, (left > right) ? 1 : 0);
      break; /* left >  right */
    case 7:
      stackPush(vm, (left >= right) ? 1 : 0);
      break; /* left >= right */
    case 8:
      stackPush(vm, left + right);
      break; /* left +  right */
    case 9:
      stackPush(vm, left - right);
      break; /* left -  right */
    case 10:
      stackPush(vm, left * right);
      break; /* left *  right */
    case 11:
      stackPush(vm, left / right);
      break; /* left /  right */
    case 12:
      stackPush(vm, left >> right);
      break; /* left >> right */
    case 13:
      stackPush(vm, left << right);
      break; /* left << right */
    case 14:
      stackPush(vm, left & right);
      break; /* left &  right */
    case 15:
      stackPush(vm, left | right);
      break; /* left |  right */
    case 16:
      stackPush(vm, left % right);
      break; /* left %  right */
    case 17:
      stackPush(vm, left ^ right);
      break; /* left ^  right */
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
    printf("0X%X ", orig);
    parseInstruction(vm, &ctx, opcode, parameter);
  }

  return 0;
}
