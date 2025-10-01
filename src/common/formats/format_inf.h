#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
  kForm = 0,
  kEmc2Ordr = 1,
  kText = 2,
  kData = 3,
  kCountChunkTypes
} ScriptChunkTypes;

typedef struct {
  uint32_t _size;
  uint8_t *_data; // by TEXT used for count of texts, by EMC2ODRD it is used
                  // for a count of somewhat
  uint8_t *_additional; // currently only used for TEXT
} ScriptChunk;

typedef struct {
  uint16_t start;
  uint16_t end;
} ScriptSegment;

typedef struct {
  ScriptChunk chunks[kCountChunkTypes];

  uint8_t *text;
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
