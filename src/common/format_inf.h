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

  uint8_t *originalBuffer;
  size_t originalBufferSize;
  ScriptSegment *segments;
  size_t segmentsNum;
} INFScript;

void INFScriptInit(INFScript *script);
void INFScriptRelease(INFScript *script);

// buffer will be owned and 'free'd by INFScriptRelease
void INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize);

// this is the number of instructions, i.e. uint16_t
size_t INFScriptGetCodeBinarySize(const INFScript *script);
const uint16_t *INFScriptGetCodeBinary(const INFScript *script);
