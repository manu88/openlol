#pragma once

#include <stddef.h>
#include <stdint.h>

// TODO check kyrandia scripts here :
// https://github.com/MrSapps/scummvm-tools/blob/6af4c033d39952939eb204c8c4f94533339f6112/engines/kyra/dekyra_v1.cpp

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

typedef struct {
  uint16_t *scriptData;
  uint32_t scriptSize;

} ScriptInfo;

#define STACK_SIZE 16
#define FRAME_POINTER_INIT 17
#define VAR_SIZE 5
typedef struct {
  ScriptInfo *scriptInfo;
  uint16_t stack[STACK_SIZE];
  uint8_t stackPointer;
  uint8_t framePointer;
  uint16_t variables[VAR_SIZE];
  uint16_t returnValue;

  // disassembler part
  uint16_t addrOffset;
  uint8_t disassemble;
  char *disasmBuffer;
  size_t disasmBufferIndex;
  size_t disasmBufferSize;
  uint8_t showDisamComment;
} ScriptVM;

void ScriptVMInit(ScriptVM *vm);
void ScriptVMSetDisassembler(ScriptVM *vm);
void ScriptVMRelease(ScriptVM *vm);
int ScriptExec(ScriptVM *vm, const ScriptInfo *info);
void ScriptVMDump(const ScriptVM *vm);
