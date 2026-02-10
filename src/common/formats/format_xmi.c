#include "format_xmi.h"
#include "bytes.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isValidXMI(uint8_t *buffer, size_t bufferSize) {
  if (bufferSize < 12) {
    return 0;
  }
  if (memcmp(buffer, "FORM", 4)) {
    return 0;
  }
  if (memcmp(buffer + 8, "XDIR", 4)) {
    return 0;
  }
  return 1;
}

static void onEvent(XMIHandle *handle, XMISequence *sequence, uint8_t *buffer,
                    size_t bufferSize) {
  sequence->events.data = buffer;
  sequence->events.dataSize = bufferSize;
}

static void onPatches(XMIHandle *handle, XMISequence *sequence, uint8_t *buffer,
                      size_t bufferSize) {
  assert(sequence->numPatches == 0);
  sequence->numPatches = bufferSize / 2;
  sequence->patches = (XMIPatch *)buffer;
}

static int readSequence(XMIHandle *handle, XMISequence *sequence,
                        uint8_t *buffer, size_t bufferSize) {
  size_t readSize = bufferSize;
  uint8_t *buff = (uint8_t *)buffer;

  if (memcmp(buff, "XMID", 4)) {
    printf("\treadSong: Expected 'XMID' chunk, got 0X%X instead \n", buff[0]);
    return 0;
  }
  buff += 4;
  readSize -= 4;

  while (readSize) {
    if (memcmp(buff, "EVNT", 4) == 0) {
      buff += 4;
      readSize -= 4;
      uint32_t eventSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize -= 4;
      onEvent(handle, sequence, buff, eventSize);
      readSize -= eventSize;
      buff += eventSize;
    } else if (memcmp(buff, "TIMB", 4) == 0) {

      readSize -= 4;
      buff += 4;

      // uint32_t timbChunkSize = swap_uint32(*(uint32_t *)buff);
      readSize -= 4;
      buff += 4;

      uint16_t count = (*(uint16_t *)buff);
      readSize -= 2;
      buff += 2;
      onPatches(handle, sequence, buff, count * 2);

      buff += count * 2;
      readSize -= count * 2;

    } else {
      printf("Unhandled chunk %x %x %x %x remains %zX\n", buff[0], buff[1],
             buff[2], buff[3], readSize);
      assert(0);
    }
  }

  return 1;
}

static int readFile(XMIHandle *handle, uint8_t *buffer, size_t bufferSize) {
  size_t readSize = bufferSize;
  uint8_t *buff = (uint8_t *)buffer;

  // get FORM:XDIR

  // already checked by isValidXMI
  assert(memcmp(buff, "FORM", 4) == 0);
  buff += 4;
  readSize -= 4;

  // uint32_t chunkSize = swap_uint32(*(uint32_t *)buff);
  buff += 4;
  readSize -= 4;

  assert(memcmp(buff, "XDIR", 4) == 0);
  buff += 4;
  readSize -= 4;

  if (memcmp(buff, "INFO", 4)) {
    printf("Expected 'INFO' chunk, got 0X%X instead \n", buff[0]);
    return 0;
  }
  buff += 4;
  readSize -= 4;
  uint32_t infoChunkSize = swap_uint32(*(uint32_t *)buff);
  assert(infoChunkSize == 2);
  buff += 4;
  readSize -= 4;
  handle->seqCount = (*(uint16_t *)buff);

  buff += 2;
  readSize -= 2;

  if (memcmp(buff, "CAT ", 4)) {
    printf("Expected 'CAT ' chunk, got 0X%X instead \n", buff[0]);
    return 0;
  }
  buff += 4;
  readSize -= 4;
  // uint32_t catChunkSize = swap_uint32(*(uint32_t *)buff);
  buff += 4;
  readSize -= 4;

  if (memcmp(buff, "XMID", 4)) {
    printf("Expected 'XMID' chunk, got 0X%X instead \n", buff[0]);
    return 0;
  }
  buff += 4;
  readSize -= 4;

  handle->sequences = malloc(handle->seqCount * sizeof(XMISequence));
  memset(handle->sequences, 0, handle->seqCount * sizeof(XMISequence));
  // one FORM:XMID subchunk for each song in the file,
  for (int i = 0; i < handle->seqCount; i++) {
    if (memcmp(buff, "FORM", 4)) {
      printf("Expected 'FORM' chunk, got 0X%X instead \n", buff[0]);
      return 0;
    }
    buff += 4;
    readSize -= 4;
    uint32_t songChunkSize = swap_uint32(*(uint32_t *)buff);
    buff += 4;
    readSize -= 4;

    if (!readSequence(handle, handle->sequences + i, buff, songChunkSize)) {
      printf("readSong: error\n");
    }

    buff += songChunkSize;
    readSize -= songChunkSize;
  }
  assert(readSize == 0);

  return 1;
}

void XMIHandleRelease(XMIHandle *handle) { free(handle->sequences); }

int XMIHandleFromBuffer(XMIHandle *handle, uint8_t *buffer, size_t bufferSize) {
  memset(handle, 0, sizeof(XMIHandle));
  if (!isValidXMI(buffer, bufferSize)) {
    return 0;
  }
  handle->data = buffer;
  handle->dataSize = bufferSize;
  return readFile(handle, buffer, bufferSize);
}
