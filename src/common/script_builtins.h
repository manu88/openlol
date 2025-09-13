#pragma once

#include <stddef.h>
#include <stdint.h>
typedef struct _EMCState EMCState;
typedef struct _EMCInterpreter EMCInterpreter;

typedef uint16_t (*ScriptFunction)(EMCInterpreter *interp, EMCState *state);

typedef struct {
  ScriptFunction fun;
  char name[32];
} ScriptFunDesc;

void EMCInterpreterExecFunction(EMCInterpreter *interp, EMCState *state,
                                uint8_t funcNum);

ScriptFunDesc *getBuiltinFunctions(void);
size_t getNumBuiltinFunctions(void);
