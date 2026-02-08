#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t patch;
  uint8_t bank;
} XMIPatch;

typedef struct {
  uint8_t *data;
  size_t dataSize;
} XMIEvents;

typedef struct {
  XMIEvents events;
  XMIPatch *patches;
  size_t numPatches;
} XMISequence;

typedef struct {
  uint16_t seqCount;
  XMISequence *sequences;
} XMIHandle;

void XMIHandleRelease(XMIHandle *handle);
int XMIHandleFromBuffer(XMIHandle *handle, uint8_t *buffer, size_t bufferSize);
