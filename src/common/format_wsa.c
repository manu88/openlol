#include "format_wsa.h"
#include "format_40.h"
#include "format_lcw.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void WSAHandleInit(WSAHandle *handle) { memset(handle, 0, sizeof(WSAHandle)); }
void WSAHandleRelease(WSAHandle *handle) {}

int WSAHandleFromBuffer(WSAHandle *handle, const uint8_t *buffer,
                        size_t bufferSize) {

  handle->header = *(WSAHeader *)buffer;
  handle->originalBuffer = (uint8_t *)buffer;
  handle->bufferSize = bufferSize;
  handle->header.frameOffsets = (uint32_t *)(handle->originalBuffer + 14);

  if (handle->header.hasPalette) {
    handle->header.palette =
        handle->originalBuffer + 14 + (handle->header.numFrames + 2) * 4;
  } else {
    handle->header.palette = NULL;
  }
  return 1;
}

uint32_t WSAHandleGetFrameOffset(const WSAHandle *handle, uint32_t index) {
  assert(index < handle->header.numFrames + 2);
  uint32_t frameOffset = handle->header.frameOffsets[index];
  return frameOffset + handle->header.hasPalette * 768;
}

uint8_t *WSAHandleGetFrame(const WSAHandle *handle, uint32_t index) {
  uint32_t offset = WSAHandleGetFrameOffset(handle, index);
  const uint8_t *frameData = handle->originalBuffer + offset;
  size_t frameSize = WSAHandleGetFrameOffset(handle, index + 1) - offset;
  size_t destSize = handle->header.delta;
  uint8_t *lcwDecompressedData = malloc(destSize);
  if (!lcwDecompressedData) {
    return NULL;
  }
  ssize_t decompressedSize =
      LCWDecompress(frameData, frameSize, lcwDecompressedData, destSize);

  size_t fullSize = handle->header.width * handle->header.height;
  uint8_t *outData = malloc(fullSize);
  if (outData) {
    Format40Decode(lcwDecompressedData, decompressedSize, outData);
  }
  free(lcwDecompressedData);
  return outData;
}
