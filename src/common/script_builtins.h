#pragma once

#include <stdint.h>
typedef struct _EMCState EMCState;
typedef struct _EMCInterpreter EMCInterpreter;

void EMCInterpreterExecFunction(EMCInterpreter *interp, EMCState *state,
                               uint8_t funcNum);
