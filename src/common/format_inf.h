#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *originalBuffer;
  size_t originalBufferSize;
  uint16_t *offsets;
  size_t offsetsNum;
} INFScript;

void INFScriptInit(INFScript *script);
void INFScriptRelease(INFScript *script);

// buffer will be owned!
void INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize);
