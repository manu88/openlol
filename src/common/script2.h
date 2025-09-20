#pragma once
#include "format_inf.h"
#include <stdint.h>

typedef struct {
  uint8_t *text;
  uint16_t *data;
  uint32_t ordrSize;
  uint16_t *ordr;
  uint32_t dataSize;

  // const Common::Array<const Opcode *> *sysFuncs;
} EMCData;

typedef struct _EMCState {
  enum { kStackSize = 100, kStackLastEntry = kStackSize - 1 };

  const uint16_t *ip;
  const EMCData *dataPtr;
  int16_t retValue;
  uint16_t bp;
  uint16_t sp;
  int16_t regs[30];          // VM registers
  int16_t stack[kStackSize]; // VM stack
} EMCState;

static inline int16_t EMCStateStackVal(const EMCState *state, uint8_t i) {
  return state->stack[state->sp + i];
}

typedef struct _EMCInterpreter {
  EMCData *_scriptData;

  int16_t _parameter;
} EMCInterpreter;

int EMCInterpreterLoad(EMCInterpreter *interp, const INFScript *infScript,
                       EMCData *data);
void EMCInterpreterUnload(EMCInterpreter *interp, EMCData *data);
void EMCStateInit(EMCState *scriptState, const EMCData *data);
int EMCInterpreterStart(EMCInterpreter *interp, EMCState *script, int function);
int EMCInterpreterIsValid(EMCInterpreter *interp, EMCState *script);
int EMCInterpreterRun(EMCInterpreter *interp, EMCState *script);

int script2Main(int argc, char *argv[]);
