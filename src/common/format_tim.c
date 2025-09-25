#include "format_tim.h"
#include "bytes.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void TIMHandleInit(TIMHandle *handle) { memset(handle, 0, sizeof(TIMHandle)); }
void TIMHandleRelease(TIMHandle *handle) {
  if (handle->originalBuffer) {
    free(handle->originalBuffer);
  }
}

int TIMHandleFromBuffer(TIMHandle *handle, const uint8_t *buffer,
                        size_t bufferSize) {
  size_t readSize = 0;
  uint8_t *buff = (uint8_t *)buffer;
  uint8_t chunkName[sizeof("TEXT") + 1];
  while (readSize < bufferSize) {
    if (bufferSize - readSize < 4) {
      break;
    }
    memcpy(chunkName, buff, 4);
    chunkName[4] = '\0';
    buff += 4;
    readSize += 4;
    if (strcmp((char *)chunkName, "FORM") == 0) {
      buff += 4;
      readSize += 4;
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      handle->text = buff;
      handle->textSize = textSize;
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "AVTL") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff); // size in bytes!
      buff += 4;
      readSize += 4;
      handle->avtlSize = dataSize / 2;
      handle->avtl = (uint16_t *)buff;

      buff += dataSize;
      readSize += dataSize;
    } else if (strcmp((char *)chunkName, "AVFS") == 0) {
      // nothing to do, the chunk is here but empty
    } else {
      printf("unknown chunk '%s'\n", chunkName);
      assert(0);
    }
  }
  if (handle->textSize < 0) {
    assert(handle->text);
  }
  handle->numFunctions = handle->avtlSize < TIM_NUM_FUNCTIONS
                             ? handle->avtlSize
                             : TIM_NUM_FUNCTIONS;
  for (int i = 0; i < handle->numFunctions; i++) {
    uint16_t offset = handle->avtl[i];

    handle->functions[i].avtl = handle->avtl + offset;
  }

  return 1;
}

const char *TIMHandleGetText(TIMHandle *handle, int index) {
  assert(index == 0); // need to implement indexing
  if (handle->textSize == 0) {
    return NULL;
  }
  return (const char *)handle->text + *(handle->text);
}
