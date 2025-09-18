#include "script.h"
#include "bytes.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ScriptFunDesc functions[];

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
  uint16_t currentWord;
} ScriptContext;

void stackPush(ScriptVM *vm, uint16_t value) {
  // printf("stackPush 0X%04X\n", value);
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

uint16_t stackPos(ScriptVM *vm, uint8_t position) {
  assert(position < STACK_SIZE);
  return vm->stack[STACK_SIZE - position];
}

#define PRINTFLIKE(n, m) __attribute__((format(printf, n, m)))

static void emitLine(ScriptVM *vm, ScriptContext *ctx, const char *fmt, ...)
    PRINTFLIKE(3, 4);

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
    printf("no more size to write line, writtenSize=%zu, got bSize=%zu\n",
           writtenSize, bSize);
    assert(0);
  }
  vm->disasmBufferIndex += writtenSize;
  va_end(args);

  if (vm->showDisamComment) {
    bSize = vm->disasmBufferSize - writtenSize;
    writtenSize =
        snprintf(vm->disasmBuffer + vm->disasmBufferIndex, bSize, "; 0X%04X\n",
                 ctx->currentAddress - ctx->scriptStart);

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

static void emitFunctionCall(ScriptVM *vm, ScriptContext *ctx,
                             uint16_t parameter) {
  if (functions[parameter].name[0]) {
    emitLine(vm, ctx, "%s %s", MNEMONIC_CALL, functions[parameter].name);
  } else {
    emitLine(vm, ctx, "%s 0X%X", MNEMONIC_CALL, parameter);
  }
}

static uint16_t invokeFunction(ScriptVM *vm, ScriptContext *ctx,
                               uint16_t funcCode);

static int parseInstruction(ScriptVM *vm, ScriptContext *ctx, uint8_t opCode,
                            uint16_t parameter) {
  switch ((ScriptCommand)opCode) {

  case OP_JUMP:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_JUMP, parameter);
    } else {
      vm->instructionPointer += parameter;
    }
    return 1;
  case OP_PUSH_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_PUSH_RC, parameter);
    } else {
      switch (parameter) {
      case 0:
        stackPush(vm, vm->returnValue);
        break;
      case 1: {
        uint32_t location = (uint32_t)vm->instructionPointer + 1;
        stackPush(vm, location);
        vm->framePointer = vm->stackPointer + 2;
        break;
      }
      }
    }
    return 1;
  case OP_PUSH:
  case OP_PUSH2:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_PUSH, parameter);
    } else {
      stackPush(vm, parameter);
    }
    return 1;
  case OP_PUSH_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_PUSH_VAR, parameter);
    } else {
      stackPush(vm, vm->variables[parameter]);
    }
    return 1;
  case OP_PUSH_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_PUSH_LOC_VAR, parameter);
    } else {
      printf("PUSH LOC VAR %X", parameter);
      assert(0);
    }
    return 1;
  case OP_PUSH_PARAMETER:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_PUSH_ARG, parameter);
    } else {
      // printf("PushArg 0X%X\n", parameter);
      stackPush(vm, vm->stack[vm->framePointer + parameter - 1]);
    }
    return 1;
  case OP_POP_RETURN_OR_LOCATION:
    assert(parameter == 1 || parameter == 0);
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_POP_RC, parameter);
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
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_POP, parameter);
    } else {
      vm->variables[parameter] = stackPop(vm);
    }
    return 1;
  case OP_POP_LOCAL_VARIABLE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_POP_LOC_VAR, parameter);
    } else {
      printf("POP LOCAL VAR %X\n", parameter);
      assert(0);
    }
    return 1;
  case OP_POP_PARAMETER:
    if (vm->disassemble) {
      emitLine(vm, ctx, "POPPARAM %X", parameter);
    } else {
      vm->stack[vm->framePointer + parameter - 1] = stackPop(vm);
    }
    return 1;
  case OP_STACK_REWIND:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_STACK_REWIND, parameter);
    } else {
      vm->stackPointer += parameter;
    }
    return 1;
  case OP_STACK_FORWARD:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_STACK_FORWARD, parameter);
    } else {
      vm->stackPointer -= parameter;
    }
    return 1;
  case OP_FUNCTION:
    if (vm->disassemble) {
      emitFunctionCall(vm, ctx, parameter);
    } else {
      parameter &= 0xFF;
      vm->returnValue = invokeFunction(vm, ctx, parameter);
    }
    return 1;
  case OP_JUMP_NE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_JUMP_NE, parameter);
    } else {
      if (!vm->stack[vm->stackPointer++]) {
        vm->instructionPointer = parameter & 0x7FFF;
      }
    }
    return 1;
  case OP_UNARY:

    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_UNARY, parameter);
    } else {
      assert(parameter == 1 || parameter == 0 || parameter == 2);
      if (parameter == 0) { /* STACK = !STACK */
        stackPush(vm, stackPop(vm) == 0 ? 1 : 0);
        return 1;
      }
      if (parameter == 1) { /* STACK = -STACK */
        stackPush(vm, -stackPop(vm));
        return 1;
      }
      if (parameter == 2) { /* STACK = ~STACK */
        stackPush(vm, ~stackPop(vm));
        return 1;
      }
    }
    return 0;
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
        emitLine(vm, ctx, MNEMONIC_LOGICAL_AND);
      } else {
        stackPush(vm, (left && right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_LogicalOR:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_LOGICAL_OR);
      } else {
        stackPush(vm, (left || right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_EQUAL:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_EQUAL);
      } else {
        stackPush(vm, (left == right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_NotEQUAL:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_NOT_EQUAL);
      } else {
        stackPush(vm, (left != right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Inf:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_INF);
      } else {
        stackPush(vm, (left < right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_InfOrEq:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_INF_EQ);
      } else {
        stackPush(vm, (left <= right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Greater:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_SUP);
      } else {
        stackPush(vm, (left > right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_GreaterOrEq:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_SUP_EQ);
      } else {
        stackPush(vm, (left >= right) ? 1 : 0);
      }
      return 1;
    case BinaryOp_Add:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_ADD);
      } else {
        stackPush(vm, left + right);
      }
      return 1;
    case BinaryOp_Minus:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_MINUS);
      } else {
        stackPush(vm, left - right);
      }
      return 1;
    case BinaryOp_Multiply:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_MULTIPLY);
      } else {
        stackPush(vm, left * right);
      }
      return 1;
    case BinaryOp_Divide:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_DIVIDE);
      } else {
        stackPush(vm, left / right);
      }
      return 1;
    case BinaryOp_RShift:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_RIGHT_SHIFT);
      } else {
        stackPush(vm, left >> right);
      }
      return 1;
    case BinaryOp_LShift:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_LEFT_SHIFT);
      } else {
        stackPush(vm, left << right);
      }
      return 1;
    case BinaryOp_AND:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_AND);
      } else {
        stackPush(vm, left & right);
      }
      return 1;
    case BinaryOp_OR:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_OR);
      } else {
        stackPush(vm, left | right);
      }
      return 1;
    case BinaryOp_MOD:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_MOD);
      } else {
        stackPush(vm, left % right);
      }
      return 1;
    case BinaryOp_XOR:
      if (vm->disassemble) {
        emitLine(vm, ctx, MNEMONIC_XOR);
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
      emitLine(vm, ctx, MNEMONIC_RET);
    } else {
      assert(0);
    }
    return 1;
  case OP_SETRETURNVALUE:
    if (vm->disassemble) {
      emitLine(vm, ctx, "%s 0X%X", MNEMONIC_SETRET, parameter);
    } else {
      assert(0);
    }
    return 1;
  default:
    printf("unknown instruction at 0X04%X opcode=%x (param=%x)\n",
           ctx->currentAddress, opCode, parameter);
  }
  return 0;
}

int ScriptExec(ScriptVM *vm, const ScriptInfo *info) {
  ScriptContext ctx = {0};
  ctx.scriptStart = vm->instructionPointer;
  while (vm->instructionPointer < info->scriptSize) {
    ctx.currentAddress = vm->instructionPointer;
    uint16_t orig = *(info->scriptData + vm->instructionPointer++);
    ctx.currentWord = orig;
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
      parameter = swap_uint16(*(info->scriptData + vm->instructionPointer++));
    }
    if (!parseInstruction(vm, &ctx, opcode, parameter)) {
      return 0;
    }
  }
  return 1;
}

static uint16_t getItemParam(ScriptVM *vm) {
  printf("ScriptVM: getItemParam\n");
  return 0;
}

static uint16_t fadeToBlack(ScriptVM *vm) {
  printf("ScriptVM: fadeToBlack\n");
  return 0;
}

static uint16_t loadBitmap(ScriptVM *vm) {
  printf("ScriptVM: fadeToBlack\n");
  return 0;
}

static uint16_t hideMouse(ScriptVM *vm) {
  printf("ScriptVM: hideMouse\n");
  return 0;
}

static uint16_t showMouse(ScriptVM *vm) {
  printf("ScriptVM: showMouse\n");
  return 0;
}

static uint16_t drawScene(ScriptVM *vm) {
  printf("ScriptVM: drawScene\n");
  return 0;
}

static uint16_t rollDice(ScriptVM *vm) {
  uint16_t p0 = stackPos(vm, 0);
  uint16_t p1 = stackPos(vm, 1);
  printf("ScriptVM: rollDice %X %X\n", p0, p1);
  return 0;
}

static uint16_t stopTimScript(ScriptVM *vm) {
  printf("ScriptVM: stopTimScript\n");
  return 0;
}

static uint16_t clearDialogueField(ScriptVM *vm) {
  printf("ScriptVM: clearDialogueField\n");
  return 0;
}

static uint16_t setupBackgroundAnimationPart(ScriptVM *vm) {
  printf("ScriptVM: clearDialogueField\n");
  return 0;
}

static uint16_t playDialogueTalkText(ScriptVM *vm) {
  uint16_t track = stackPos(vm, 0);
  printf("ScriptVM: playDialogueTalkText track=0X%X\n", track);
  return 0;
}

static uint16_t loadTimScript(ScriptVM *vm) {
  printf("ScriptVM: loadTimScript\n");
  return 0;
}

static uint16_t runTimScript(ScriptVM *vm) {
  printf("ScriptVM: runTimScript\n");
  return 0;
}

static uint16_t releaseTimScript(ScriptVM *vm) {
  printf("ScriptVM: releaseTimScript\n");
  return 0;
}

static uint16_t testGameFlag(ScriptVM *vm) {
  uint16_t p = stackPos(vm, 0);
  printf("ScriptVM: testGameFlag %X\n", p);
  return 0;
}

static uint16_t setGameFlag(ScriptVM *vm) {
  printf("ScriptVM: setGameFlag\n");
  return 0;
}

static uint16_t checkMonsterTypeHostility(ScriptVM *vm) {
  printf("ScriptVM: checkMonsterTypeHostility\n");
  return 0;
}

static uint16_t getDirection(ScriptVM *vm) {
  printf("ScriptVM: getDirection\n");
  return 0;
}

static uint16_t playCharacterScriptChat(ScriptVM *vm) {
  printf("ScriptVM: playCharacterScriptChat\n");
  return 0;
}

static uint16_t moveMonster(ScriptVM *vm) {
  uint8_t monsterId = vm->stack[0];
  uint8_t numBlocks = vm->stack[1];
  uint8_t xOffs = vm->stack[2];
  uint8_t yOffs = vm->stack[3];
  printf("ScriptVM: moveMonster %i numBlocs %i xOffs %i yOffs %i\n", monsterId,
         numBlocks, xOffs, yOffs);
  return 1;
}

static uint16_t setGlobalVar(ScriptVM *vm) {
  uint16_t p0 = stackPos(vm, 0);
  uint16_t p1 = stackPos(vm, 1);
  uint16_t p2 = stackPos(vm, 2);

  printf("ScriptVM: setGlobalVar %X %X %X\n", p0, p1, p2);
  return 0;
}

static uint16_t printMessage(ScriptVM *vm) {
  printf("ScriptVM: printMessage\n");
  return 0;
}

static uint16_t savePage5(ScriptVM *vm) {
  printf("ScriptVM: savePage5\n");
  return 1;
}

static uint16_t prepareSpecialScene(ScriptVM *vm) {
  printf("ScriptVM: prepareSpecialScene\n");
  return 1;
}

static uint16_t copyRegion(ScriptVM *vm) {
  printf("ScriptVM: copyRegion\n");
  return 1;
}

static uint16_t drawExitButton(ScriptVM *vm) {
  printf("ScriptVM: drawExitButton\n");
  return 1;
}

static ScriptFunDesc functions[] = {

    {NULL},
    {NULL},
    {drawScene, "drawScene"},
    {rollDice, "rollDice"},
    {NULL},
    {NULL},
    {NULL},
    {setGameFlag, "setGameFlag"},
    {testGameFlag, "testGameFlag"},
    {NULL},
    // 0X0A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X10
    {NULL},
    {NULL},
    {getItemParam, "getItemParam"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X1A
    {NULL},
    {NULL},
    {getDirection, "getDirection"},
    {NULL},

    {NULL},
    {NULL},

    // 0X20
    {NULL},
    {NULL},
    {clearDialogueField, "clearDialogueField"},
    {setupBackgroundAnimationPart, "setupBackgroundAnimationPart"},
    {NULL},
    {hideMouse, "hideMouse"},
    {showMouse, "showMouse"},
    {fadeToBlack, "fadeToBlack"},
    {NULL},
    {loadBitmap, "loadBitmap"},
    // 0X2A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X30
    {setGlobalVar, "setGlobalVar"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {copyRegion, "copyRegion"},
    {NULL},
    {NULL},
    // 0X3A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X40
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {moveMonster, "moveMonster"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X4A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {loadTimScript, "loadTimScript"},
    {runTimScript, "runTimScript"},

    // 0X50
    {releaseTimScript, "releaseTimScript"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {stopTimScript, "stopTimScript"},
    // 0X5A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {playCharacterScriptChat, "playCharacterScriptChat"},
    {NULL},

    // 0X60
    {NULL},
    {NULL},
    {drawExitButton, "drawExitButton"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X6A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {printMessage, "printMessage"},

    // 0X70
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X7A
    {playDialogueTalkText, "playDialogueTalkText"},
    {checkMonsterTypeHostility, "checkMonsterTypeHostility"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X80
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X8A
    {savePage5, "savePage5"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X90
    {NULL},
    {NULL},
    {prepareSpecialScene, "prepareSpecialScene"},
};

static const size_t numFunctions = sizeof(functions) / sizeof(ScriptFunDesc);

const ScriptFunDesc *ScriptGetBuiltinFunctions(size_t *numFunctions) {
  assert(numFunctions);
  *numFunctions = sizeof(functions) / sizeof(ScriptFunDesc);
  return functions;
}

// see
// https://github.com/scummvm/scummvm/blob/master/engines/kyra/script/script_lol.cpp#L2683
static uint16_t invokeFunction(ScriptVM *vm, ScriptContext *ctx,
                               uint16_t funcCode) {
  if (funcCode >= numFunctions) {
    printf("FUNCTION 0X%X/%zX\n", funcCode, numFunctions);
  }
  assert(funcCode < numFunctions);
  ScriptFunDesc funDesc = functions[funcCode];
  if (!funDesc.fun) {
    printf("FUNCTION 0X%X/%zX\n", funcCode, numFunctions);
  }
  assert(funDesc.fun);
  return funDesc.fun(vm);
}
