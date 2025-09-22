#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t numFrames;
  uint16_t xPos;
  uint16_t yPos;
  uint16_t width;
  uint16_t height;

  uint16_t delta;
  uint16_t hasPalette;

  uint32_t *frameOffsets; // size is numFrames + 2

  uint8_t *palette; // VGA palette, so 768 values
} WSAHeader;

typedef struct {
  WSAHeader header;
  uint8_t *originalBuffer;
} WSAHandle;

void WSAHandleInit(WSAHandle *handle);
void WSAHandleRelease(WSAHandle *handle);
int WSAHandleFromBuffer(WSAHandle *handle, const uint8_t *buffer,
                        size_t bufferSize);
uint8_t *WSAHandleGetFrameOffset(const WSAHandle *handle, uint32_t index);
