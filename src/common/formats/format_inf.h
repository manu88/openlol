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

  uint16_t *data;
  uint32_t ordrSize;
  uint16_t *ordr;
  uint32_t dataSize;

} INFScript;

void INFScriptInit(INFScript *script);
void INFScriptRelease(INFScript *script);

int INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize);

// returns -1 if no  offset exists for functionNum
int INFScriptGetFunctionOffset(const INFScript *script, uint16_t functionNum);
int INFScriptGetNumFunctions(const INFScript *script);

const char *INFScriptGetDataString(const INFScript *script, int16_t index);
