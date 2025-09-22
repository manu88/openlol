#include "format_wsa.h"
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
  return 0;
}
