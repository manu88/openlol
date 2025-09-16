#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {

  uint16_t count;

  uint8_t *originalBuffer;
} LangHandle;

void LangHandleRelease(LangHandle *handle);
int LangHandleFromBuffer(LangHandle *handle, uint8_t *buffer,
                         size_t bufferSize);
void LangHandleShow(LangHandle *handle);
uint16_t LangHandleGetString(const LangHandle *handle, uint16_t index,
                             char *outBuffer);
