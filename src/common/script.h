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

typedef enum {
  EMCGetItemParam_Block = 0,
  EMCGetItemParam_X = 1,
  EMCGetItemParam_Y = 2,
  EMCGetItemParam_LEVEL = 3,
  EMCGetItemParam_PropertyIndex = 4,
  EMCGetItemParam_CurrentFrameFlag = 5,
  EMCGetItemParam_NameStringId = 6,
  EMCGetItemParam_UNUSED_7 = 7,
  EMCGetItemParam_ShpIndex = 8,
  EMCGetItemParam_Type = 9,
  EMCGetItemParam_ScriptFun = 10,
  EMCGetItemParam_Might = 11,
  EMCGetItemParam_Skill = 12,
  EMCGetItemParam_Protection = 13,
  EMCGetItemParam_14 = 14,
  EMCGetItemParam_CurrentFrameFlag2 = 15,
  EMCGetItemParam_Flags = 16,
  EMCGetItemParam_SkillMight = 17,
} EMCGetItemParam;

typedef struct _EMCInterpreter EMCInterpreter;

typedef struct _EMCInterpreterCallbacks {
  uint16_t (*EMCInterpreterCallbacks_SetGlobalVar)(EMCInterpreter *interp,
                                                   EMCGlobalVarID id,
                                                   uint16_t a, uint16_t b);
  uint16_t (*EMCInterpreterCallbacks_GetGlobalVar)(EMCInterpreter *interp,
                                                   EMCGlobalVarID id,
                                                   uint16_t a);
  uint16_t (*EMCInterpreterCallbacks_GetDirection)(EMCInterpreter *interp);

  // charId = 1 means selected char
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
  void (*EMCInterpreterCallbacks_ClearDialogField)(EMCInterpreter *interp);
  // files: VCF VCN VMP
  void (*EMCInterpreterCallbacks_LoadLevelGraphics)(EMCInterpreter *interp,
                                                    const char *file,
                                                    const char *paletteFile);
  void (*EMCInterpreterCallbacks_LoadLevel)(EMCInterpreter *interp,
                                            uint16_t levelNum,
                                            uint16_t startBlock,
                                            uint16_t startDir);
  uint16_t (*EMCInterpreterCallbacks_TestGameFlag)(EMCInterpreter *interp,
                                                   uint16_t flag);
  void (*EMCInterpreterCallbacks_SetGameFlag)(EMCInterpreter *interp,
                                              uint16_t flag, uint16_t set);
  void (*EMCInterpreterCallbacks_LoadBitmap)(EMCInterpreter *interp,
                                             const char *file, uint16_t param);
  void (*EMCInterpreterCallbacks_LoadDoorShapes)(EMCInterpreter *interp,
                                                 const char *file, uint16_t p1,
                                                 uint16_t p2, uint16_t p3,
                                                 uint16_t p4);
  void (*EMCInterpreterCallbacks_LoadMonsterShapes)(EMCInterpreter *interp,
                                                    const char *file,
                                                    uint16_t monsterId,
                                                    uint16_t p2);
  void (*EMCInterpreterCallbacks_LoadMonster)(
      EMCInterpreter *interp, uint16_t monsterIndex, uint16_t shapeIndex,
      uint16_t hitChance, uint16_t protection, uint16_t evadeChance,
      uint16_t speed, uint16_t p6, uint16_t p7, uint16_t p8);
  void (*EMCInterpreterCallbacks_LoadTimScript)(EMCInterpreter *interp,
                                                uint16_t scriptId,
                                                const char *file);
  void (*EMCInterpreterCallbacks_RunTimScript)(EMCInterpreter *interp,
                                               uint16_t scriptId,
                                               uint16_t loop);
  void (*EMCInterpreterCallbacks_ReleaseTimScript)(EMCInterpreter *interp,
                                                   uint16_t scriptId);
  uint16_t (*EMCInterpreterCallbacks_GetItemIndexInHand)(
      EMCInterpreter *interp);
  void (*EMCInterpreterCallbacks_AllocItemProperties)(EMCInterpreter *interp,
                                                      uint16_t size);
  void (*EMCInterpreterCallbacks_SetItemProperty)(
      EMCInterpreter *interp, uint16_t index, uint16_t stringId,
      uint16_t shapeId, uint16_t type, uint16_t scriptFun, uint16_t might,
      uint16_t skill, uint16_t protection, uint16_t flags);
  uint16_t (*EMCInterpreterCallbacks_CheckMonsterHostility)(
      EMCInterpreter *interp, uint16_t monsterType);
  uint16_t (*EMCInterpreterCallbacks_GetItemParam)(EMCInterpreter *interp,
                                                   uint16_t itemId,
                                                   EMCGetItemParam how);
  void (*EMCInterpreterCallbacks_DisableControls)(EMCInterpreter *interp,
                                                  uint16_t mode);
  void (*EMCInterpreterCallbacks_EnableControls)(EMCInterpreter *interp);
  uint16_t (*EMCInterpreterCallbacks_GetGlobalScriptVar)(EMCInterpreter *interp,
                                                         uint16_t index);
  void (*EMCInterpreterCallbacks_SetGlobalScriptVar)(EMCInterpreter *interp,
                                                     uint16_t index,
                                                     uint16_t val);
  void (*EMCInterpreterCallbacks_WSAInit)(EMCInterpreter *interp,
                                          uint16_t index, const char *wsaFile,
                                          int x, int y, int offscreen,
                                          int flags);
  void (*EMCInterpreterCallbacks_InitSceneDialog)(EMCInterpreter *interp,
                                                  int controlMode);

  void (*EMCInterpreterCallbacks_CopyPage)(EMCInterpreter *interp,
                                           uint16_t srcX, uint16_t srcY,
                                           uint16_t destX, uint16_t destY,
                                           uint16_t w, uint16_t h,
                                           uint16_t srcPage, uint16_t dstPage);

  void (*EMCInterpreterCallbacks_DrawExitButton)(EMCInterpreter *interp,
                                                 uint16_t p0, uint16_t p1);
  void (*EMCInterpreterCallbacks_RestoreAfterSceneDialog)(
      EMCInterpreter *interp, int controlMode);
  void (*EMCInterpreterCallbacks_RestoreAfterSceneWindowDialog)(
      EMCInterpreter *interp, int redraw);

  uint16_t (*EMCInterpreterCallbacks_GetWallType)(EMCInterpreter *interp,
                                                  uint16_t blockId,
                                                  uint16_t wall);
  void (*EMCInterpreterCallbacks_SetWallType)(EMCInterpreter *interp,
                                              uint16_t blockId, uint16_t wall,
                                              uint16_t val);
  uint16_t (*EMCInterpreterCallbacks_GetWallFlags)(EMCInterpreter *interp,
                                                   uint16_t blockId,
                                                   uint16_t wall);

  uint16_t (*EMCInterpreterCallbacks_CheckRectForMousePointer)(
      EMCInterpreter *interp, uint16_t xMin, uint16_t yMin, uint16_t xMax,
      uint16_t yMax);

  void (*EMCInterpreterCallbacks_SetupDialogueButtons)(EMCInterpreter *interp,
                                                       uint16_t numStrs,
                                                       uint16_t strIds[3]);

  uint16_t (*EMCInterpreterCallbacks_ProcessDialog)(EMCInterpreter *interp);

  void (*EMCInterpreterCallbacks_SetupBackgroundAnimationPart)(
      EMCInterpreter *interp, uint16_t animIndex, uint16_t part,
      uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles,
      uint16_t nextPart, uint16_t partDelay, uint16_t field, uint16_t sfxIndex,
      uint16_t sfxFrame);

  void (*EMCInterpreterCallbacks_DeleteHandItem)(EMCInterpreter *interp);
  uint16_t (*EMCInterpreterCallbacks_CreateHandItem)(EMCInterpreter *interp,
                                                     uint16_t itemType,
                                                     uint16_t p1, uint16_t p2);

  void (*EMCInterpreterCallbacks_PlayAnimationPart)(EMCInterpreter *interp,
                                                    uint16_t animIndex,
                                                    uint16_t firstFrame,
                                                    uint16_t lastFrame,
                                                    uint16_t delay);

  uint16_t (*EMCInterpreterCallbacks_CheckForCertainPartyMember)(
      EMCInterpreter *interp, uint16_t charId);

  void (*EMCInterpreterCallbacks_SetNextFunc)(EMCInterpreter *interp,
                                              uint16_t func);

  uint16_t (*EMCInterpreterCallbacks_GetCredits)(EMCInterpreter *interp);
  void (*EMCInterpreterCallbacks_CreditsTransaction)(EMCInterpreter *interp,
                                                     int16_t amount);

  void (*EMCInterpreterCallbacks_MoveMonster)(EMCInterpreter *interp,
                                              uint16_t monsterId,
                                              uint16_t destBlock, uint16_t xOff,
                                              uint16_t yOff, uint16_t destDir);
  void (*EMCInterpreterCallbacks_PlaySoundFX)(EMCInterpreter *interp,
                                              uint16_t soundId);

  void (*EMCInterpreterCallbacks_CharacterSurpriseSFX)(EMCInterpreter *interp);
  void (*EMCInterpreterCallbacks_MoveParty)(EMCInterpreter *interp,
                                            uint16_t how);
  void (*EMCInterpreterCallbacks_FadeScene)(EMCInterpreter *interp,
                                            uint16_t mode);

  void (*EMCInterpreterCallbacks_PrepareSpecialScene)(
      EMCInterpreter *interp, uint16_t fieldType, uint16_t hasDialogue,
      uint16_t suspendGUI, uint16_t allowSceneUpdate, uint16_t controlMode,
      uint16_t fadeFlag);
  void (*EMCInterpreterCallbacks_RestoreAfterSpecialScene)(
      EMCInterpreter *interp, uint16_t fadeFlag, uint16_t redrawPlayField,
      uint16_t releaseTimScripts, uint16_t sceneUpdateMode);
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
