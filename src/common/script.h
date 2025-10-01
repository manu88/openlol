#pragma once
#include "formats/format_inf.h"
#include "script_disassembler.h"
#include <stdint.h>

// from https://github.com/OpenDUNE/OpenDUNE/blob/master/src/script/script.h

typedef enum {
  OP_JUMP = 0X00, // JUMP instruction given by the parameter.
  OP_SETRETURNVALUE =
      0X01, // Set the return value to the value given by the parameter.
  OP_PUSH_RETURN_OR_LOCATION =
      0X02,        // Push the return value (parameter = 0) or the location +
                   // framepointer (parameter = 1) on the stack.
  OP_PUSH = 0X03,  // Push a value given by the parameter on the stack.
  OP_PUSH2 = 0X04, // Identical to SCRIPT_PUSH.
  OP_PUSH_VARIABLE = 0X05,       // Push a variable on the stack.
  OP_PUSH_LOCAL_VARIABLE = 0X06, // Push a local variable on the stack.
  OP_PUSH_PARAMETER = 0X07,      // Push a parameter on the stack.
  OP_POP_RETURN_OR_LOCATION =
      0X08, // Pop the return value (parameter = 0) or the location +
            // framepointer (parameter = 1) from the stack.
  OP_POP_VARIABLE = 0X09,       // Pop a variable from the stack.
  OP_POP_LOCAL_VARIABLE = 0X0A, // Pop a local variable from the stack.
  OP_POP_PARAMETER = 0X0B,      // Pop a parameter from the stack.
  OP_STACK_REWIND =
      0X0C, // Add a value given by the parameter to the stackpointer.
  OP_STACK_FORWARD =
      0X0D, // Substract a value given by the parameter to the stackpointer.
  OP_FUNCTION = 0X0E, // Execute a function by its ID given by the parameter.
  OP_JUMP_NE = 0X0F,  // Jump to the instruction given by the parameter if the
                      // last entry on the stack is non-zero.
  OP_UNARY = 0X10,    // Perform unary operations.
  OP_BINARY = 0X11,   // Perform binary operations.
  OP_RETURN = 0X12    // Return from a subroutine.
} ScriptCommand;

typedef enum {
  BinaryOp_LogicalAND = 0,
  BinaryOp_LogicalOR = 1,
  BinaryOp_EQUAL = 2,
  BinaryOp_NotEQUAL = 3,
  BinaryOp_Inf = 4,
  BinaryOp_InfOrEq = 5,
  BinaryOp_Greater = 6,
  BinaryOp_GreaterOrEq = 7,
  BinaryOp_Add = 8,
  BinaryOp_Minus = 9,
  BinaryOp_Multiply = 10,
  BinaryOp_Divide = 11,
  BinaryOp_RShift = 12,
  BinaryOp_LShift = 13,
  BinaryOp_AND = 14,
  BinaryOp_OR = 15,
  BinaryOp_MOD = 16,
  BinaryOp_XOR = 17,
} BinaryOp;

#define MNEMONIC_JUMP (const char *)"JUMP"
#define MNEMONIC_RET (const char *)"RET"
#define MNEMONIC_PUSH (const char *)"PUSH"
#define MNEMONIC_PUSH_ARG (const char *)"PUSHARG"
#define MNEMONIC_PUSH_VAR (const char *)"PUSHVAR"
#define MNEMONIC_PUSH_LOC_VAR (const char *)"PUSHLOCVAR"

#define MNEMONIC_POP (const char *)"POP"
#define MNEMONIC_POP_LOC_VAR (const char *)"POPLOCVAR"

#define MNEMONIC_SETRET (const char *)"SETRET"
#define MNEMONIC_MULTIPLY (const char *)"MULTIPLY"
#define MNEMONIC_ADD (const char *)"ADD"
#define MNEMONIC_DIVIDE (const char *)"DIVIDE"
#define MNEMONIC_PUSH_RC (const char *)"PUSHRC"
#define MNEMONIC_POP_RC (const char *)"POPRC"
#define MNEMONIC_CALL (const char *)"CALL"
#define MNEMONIC_JUMP_NE (const char *)"IFNOTGO"
#define MNEMONIC_STACK_REWIND (const char *)"STACKRWD"
#define MNEMONIC_STACK_FORWARD (const char *)"STACKFWD"
#define MNEMONIC_UNARY (const char *)"UNARY"

#define MNEMONIC_LOGICAL_OR (const char *)"LOR"
#define MNEMONIC_LOGICAL_AND (const char *)"LAND"
#define MNEMONIC_LEFT_SHIFT (const char *)"LSHIFT"
#define MNEMONIC_RIGHT_SHIFT (const char *)"RSHIFT"
#define MNEMONIC_XOR (const char *)"XOR"
#define MNEMONIC_MOD (const char *)"MOD"
#define MNEMONIC_OR (const char *)"OR"
#define MNEMONIC_AND (const char *)"AND"
#define MNEMONIC_INF (const char *)"INF"
#define MNEMONIC_INF_EQ (const char *)"INFEQ"
#define MNEMONIC_SUP (const char *)"SUP"
#define MNEMONIC_SUP_EQ (const char *)"SUPEQ"
#define MNEMONIC_MINUS (const char *)"MINUS"
#define MNEMONIC_EQUAL (const char *)"EQUAL"
#define MNEMONIC_NOT_EQUAL (const char *)"NEQUAL"

typedef struct _EMCState {
  enum { kStackSize = 100, kStackLastEntry = kStackSize - 1 };

  const uint16_t *ip;
  const INFScript *dataPtr;
  int16_t retValue;
  uint16_t bp;
  uint16_t sp;
  int16_t regs[30];          // VM registers
  int16_t stack[kStackSize]; // VM stack
} EMCState;

static inline int16_t EMCStateStackVal(const EMCState *state, uint8_t i) {
  return state->stack[state->sp + i];
}

const char *EMCStateGetDataString(const EMCState *state, int16_t index);

typedef enum {
  EMCGlobalVarID_CurrentBlock = 0,
  EMCGlobalVarID_CurrentDir = 1,
  EMCGlobalVarID_CurrentLevel = 2,
  EMCGlobalVarID_ItemInHand = 3,
  EMCGlobalVarID_Brightness = 4,
  EMCGlobalVarID_Credits = 5,
  EMCGlobalVarID_6 = 6,
  EMCGlobalVarID_7_Unused = 7,
  EMCGlobalVarID_UpdateFlags = 8,
  EMCGlobalVarID_OilLampStatus = 9,
  EMCGlobalVarID_SceneDefaultUpdate = 10,
  EMCGlobalVarID_CompassBroken = 11,
  EMCGlobalVarID_DrainMagic = 12,
  EMCGlobalVarID_SpeechVolume = 13,
  EMCGlobalVarID_AbortTIMFlag = 14,
} EMCGlobalVarID;

typedef struct _EMCInterpreter EMCInterpreter;

typedef struct _EMCInterpreterCallbacks {
  uint16_t (*EMCInterpreterCallbacks_SetGlobalVar)(EMCInterpreter *interp,
                                                   EMCGlobalVarID id,
                                                   uint16_t a, uint16_t b);
  uint16_t (*EMCInterpreterCallbacks_GetGlobalVar)(EMCInterpreter *interp,
                                                   EMCGlobalVarID id,
                                                   uint16_t a);
  uint16_t (*EMCInterpreterCallbacks_GetDirection)(EMCInterpreter *interp);
  void (*EMCInterpreterCallbacks_PlayDialogue)(EMCInterpreter *interp,
                                               int16_t charId, int16_t mode,
                                               uint16_t strId);
  void (*EMCInterpreterCallbacks_PrintMessage)(EMCInterpreter *interp,
                                               uint16_t type, uint16_t strId,
                                               uint16_t soundId);
  void (*EMCInterpreterCallbacks_LoadLangFile)(EMCInterpreter *interp,
                                               const char *file);
  void (*EMCInterpreterCallbacks_LoadCMZ)(EMCInterpreter *interp,
                                          const char *file);
  void (*EMCInterpreterCallbacks_LoadLevelShapes)(EMCInterpreter *interp,
                                                  const char *shpFile,
                                                  const char *datFile);

  // files: VCF VCN VMP
  void (*EMCInterpreterCallbacks_LoadLevelGraphics)(EMCInterpreter *interp,
                                                    const char *file);
} EMCInterpreterCallbacks;

typedef struct _EMCInterpreter {
  INFScript *_scriptData;
  EMCDisassembler *disassembler;

  EMCInterpreterCallbacks callbacks;
  void *callbackCtx;

} EMCInterpreter;

void EMCInterpreterUnload(EMCInterpreter *interp);
void EMCStateInit(EMCState *scriptState, const INFScript *script);
int EMCStateGetFunctionOffset(const EMCState *script, uint16_t functionNum);
int EMCStateSetOffset(EMCState *script, uint16_t offset);
int EMCStateStart(EMCState *script, int function);
int EMCInterpreterIsValid(EMCInterpreter *interp, EMCState *state);
int EMCInterpreterRun(EMCInterpreter *interp, EMCState *state);
