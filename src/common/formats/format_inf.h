#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t start;
  uint16_t end;
} ScriptSegment;

typedef struct {
  uint8_t *text;
  size_t textSize;

  int numTextStrings;

  uint32_t numOffsets;
  uint16_t *functionOffsets;

  uint16_t *scriptData;
  uint32_t scriptDataSize;
} INFScript;

void INFScriptInit(INFScript *script);

int INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize);

// returns -1 if no offset exists for functionNum
int INFScriptGetFunctionOffset(const INFScript *script, uint16_t functionNum);
int INFScriptGetNumFunctions(const INFScript *script);

// returns the offset index, or -1 if no offset exists for functionNum
int INFScriptIsOffset(const INFScript *script, uint16_t instOffset);

const char *INFScriptGetDataString(const INFScript *script, int16_t index);
