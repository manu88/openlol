#include "format_wsa.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void WSAHandleInit(WSAHandle *handle) { memset(handle, 0, sizeof(WSAHandle)); }
void WSAHandleRelease(WSAHandle *handle) {
  if (handle->originalBuffer) {
    free(handle->originalBuffer);
  }
}

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
