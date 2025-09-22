#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {

  uint8_t *originalBuffer;
} WSAHandle;

void WSAHandleInit(WSAHandle *handle);
void WSAHandleRelease(WSAHandle *handle);
int WSAHandleFromBuffer(WSAHandle *handle, const uint8_t *buffer,
                        size_t bufferSize);
