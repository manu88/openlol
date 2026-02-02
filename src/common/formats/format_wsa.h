#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum {
  WSA_OFFSCREEN_DECODE = 0x1,
  WSA_NO_LAST_FRAME = 0x2,
  WSA_NO_FIRST_FRAME = 0x4,
  WSA_FLIPPED = 0x8,
  WSA_HAS_PALETTE = 0x10,
  WSA_XOR = 0x20
} WSAFlags;

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
  size_t bufferSize;
} WSAHandle;

void WSAHandleInit(WSAHandle *handle);

int WSAHandleFromBuffer(WSAHandle *handle, const uint8_t *buffer,
                        size_t bufferSize);
uint32_t WSAHandleGetFrameOffset(const WSAHandle *handle, uint32_t index);

/*
Decode the frame and xor it into frameBuffer.
frameBuffer size must be handle->header.width * handle->header.height;
*/
int WSAHandleGetFrame(const WSAHandle *handle, uint32_t index,
                      uint8_t *frameBuffer, uint8_t xor);
