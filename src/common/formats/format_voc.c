#include "format_voc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char signature[] = "Creative Voice File";
int VOCHandleFromBuffer(VOCHandle *handle, const uint8_t *data,
                        size_t dataSize) {
  VOCHeader *header = (VOCHeader *)data;
  if (memcmp(header->signature, signature, 19) != 0) {
    return 0;
  }
  if (header->sig != 0X1A) {
    return 0;
  }
  handle->header = header;
  handle->firstBlock = (const VOCBlock *)(data + header->headerSize);

  return 1;
}

const uint8_t *VOCBlockGetData(const VOCBlock *block) {
  return (uint8_t *)(block) + 4;
}

int VOCBlockIsLast(const VOCBlock *block) {
  const uint8_t *next = VOCBlockGetData(block) + VOCBlockGetSize(block);
  return next != 0;
}

const VOCBlock *VOCHandleGetNextBlock(const VOCHandle *handle,
                                      const VOCBlock *block) {
  if (VOCBlockIsLast(block)) {
    return NULL;
  }
  const uint8_t *next = VOCBlockGetData(block) + VOCBlockGetSize(block);
  return (const VOCBlock *)next;
}
