#include "format_inf.h"
#include "bytes.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Adapted from
// https://github.com/scummvm/scummvm-tools/blob/cd2a721f48460b7c8f69024dbe5f26971491599c/dekyra.cpp
// https://github.com/OpenDUNE/OpenDUNE/blob/master/src/script/script.c

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

typedef enum {
  kForm = 0,
  kEmc2Ordr = 1,
  kText = 2,
  kData = 3,
  kCountChunkTypes
} ScriptChunkTypes;

typedef struct {
  uint32_t _size;
  uint8_t *_data; // by TEXT used for count of texts, by EMC2ODRD it is used
                  // for a count of somewhat
  uint8_t *_additional; // currently only used for TEXT
} ScriptChunk;

ScriptChunk _chunks[kCountChunkTypes];
uint32_t _scriptSize;

static void decodeTextArea(void) {
  printf("TEXT chunk:\n");
  // first is size
  for (uint32_t pos = 1; pos < (_chunks[kText]._size << 1); ++pos) {
    if (swap_uint16(((uint16_t *)_chunks[kText]._data)[pos]) >= _scriptSize) {
      break;
    }
    uint32_t startoffset = swap_uint16(((uint16_t *)_chunks[kText]._data)[pos]);
    printf("Index: %X Offset: %X:\n", pos, startoffset);
    /*uint32_t endoffset = TO_BE_16(((uint16_t*)_chunks[kText]._data)[pos+1]);
    printf("\nstartoffset = %d, endoffset = %d\n\n", startoffset, endoffset);
    for (; startoffset < endoffset; ++startoffset) {
            printf("%c", *(char*)(_chunks[kText]._additional + startoffset));
    }
    printf("\n");*/
    printf("%d(%d) : %s\n", pos, startoffset,
           (char *)(_chunks[kText]._data + startoffset));
  }
}

static int decodeScript(void) {
  uint16_t *script = (uint16_t *)_chunks[kData]._data;

  uint32_t scriptSize = _chunks[kData]._size / 2;
  ScriptInfo info = {0};
  info.scriptData = script;
  info.scriptSize = scriptSize;

  printf("---- start script ---- \n");
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptExec(&vm, &info);
  ScriptVMRelease(&vm);
  return 1;
}

void TestINF(const uint8_t *buffer, size_t bufferSize) {
  printf("TestINF\n");
  _scriptSize = bufferSize;
  printf("_scriptSize = %u\n", _scriptSize);
  uint8_t chunkName[sizeof("EMC2ORDR") + 1];

  size_t readSize = 0;
  uint8_t *buff = (uint8_t *)buffer;

  while (readSize < bufferSize) {
    memcpy(chunkName, buff, 4);
    chunkName[4] = '\0';
    buff += 4;
    readSize += 4;
    if (strcmp((char *)chunkName, "FORM") == 0) {
      // looks like 'FORM' is the beginning of the file.
      // size if the whole file size
      _chunks[kForm]._size = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      printf("Got a FORM chunk size %u\n", _chunks[kForm]._size);
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      textSize += textSize % 2 != 0 ? 1 : 0;
      buff += 4;
      readSize += 4;
      printf("Got a TEXT chunk size %u\n", textSize);
      _chunks[kText]._data = buff;
      _chunks[kText]._size = swap_uint32(*(uint32_t *)buff) >> 1;
      _chunks[kText]._additional =
          _chunks[kText]._data + (_chunks[kText]._size << 1);
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "DATA") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      _chunks[kData]._size = dataSize;
      _chunks[kData]._data = buff;
      printf("Got a DATA chunk size %u\n", dataSize);
      // mostly it will be the end of the file because all files should end
      // with a 'DATA' chunk
      buff += dataSize;
      readSize += dataSize;
    } else {
      memcpy(&chunkName[4], buff, 4);
      chunkName[8] = '\0';
      buff += 4;
      readSize += 4;
      if (strcmp((char *)chunkName, "EMC2ORDR") == 0) {
        uint32_t chunkSize = swap_uint32(*(uint32_t *)buff);
        _chunks[kEmc2Ordr]._size = chunkSize >> 1;
        buff += 4;
        readSize += 4;
        _chunks[kEmc2Ordr]._data = buff;
        printf("Got a EMC2ORDR chunk size %u\n", chunkSize);
        buff += chunkSize;
        readSize += chunkSize;
      } else {
        printf("unknown chunk '%s'\n", chunkName);
      }
    }
  }

  decodeTextArea();
  decodeScript();
}
