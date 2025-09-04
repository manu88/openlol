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

void INFScriptInit(INFScript *script) { memset(script, 0, sizeof(INFScript)); }
void INFScriptRelease(INFScript *script) {
  free(script->originalBuffer);
  free(script->offsets);
}

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

static void decodeTextArea(INFScript *script, ScriptChunk *chunks) {
  printf("TEXT chunk:\n");

  // first is size
  for (uint32_t pos = 1; pos < (chunks[kText]._size << 1); ++pos) {
    if (swap_uint16(((uint16_t *)chunks[kText]._data)[pos]) >=
        script->originalBufferSize) {
      break;
    }
    uint32_t startoffset = swap_uint16(((uint16_t *)chunks[kText]._data)[pos]);
    printf("Index: %X Offset: %X:\n", pos, startoffset);
    /*uint32_t endoffset = TO_BE_16(((uint16_t*)_chunks[kText]._data)[pos+1]);
    printf("\nstartoffset = %d, endoffset = %d\n\n", startoffset, endoffset);
    for (; startoffset < endoffset; ++startoffset) {
            printf("%c", *(char*)(_chunks[kText]._additional + startoffset));
    }
    printf("\n");*/
    printf("%d(%d) : %s\n", pos, startoffset,
           (char *)(chunks[kText]._data + startoffset));
  }
}

static int decodeScript(INFScript *script, ScriptChunk *chunks) {
  uint16_t *scriptData = (uint16_t *)chunks[kData]._data;
  uint32_t scriptSize = chunks[kData]._size / 2;
  ScriptInfo info = {0};
  info.scriptData = scriptData;
  info.scriptSize = scriptSize;

  printf("---- start script ---- \n");
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptVMSetDisassembler(&vm);
  ScriptExec(&vm, &info);
  printf("Wrote %zi bytes of assembly code\n", vm.disasmBufferIndex);
  printf("'%s'\n", vm.disasmBuffer);
  ScriptVMRelease(&vm);
  return 1;
}

static void decodeORDRScriptFunctions(INFScript *script, ScriptChunk *chunks) {
  assert(script->offsets == NULL);
  script->offsets = malloc(128 * sizeof(uint16_t));
  int offsetsIndex = 0;
  const uint16_t *b = (const uint16_t *)chunks[kEmc2Ordr]._data;
  for (int i = 0; i < chunks[kEmc2Ordr]._size; i++) {
    if (b[i] != 0XFFFF) {
      uint16_t offset = swap_uint16(b[i]);
      assert(offsetsIndex < 128);
      script->offsets[offsetsIndex++] = offset;
    }
  }
  printf("Got %i offsets\n", offsetsIndex);
  for (int i = 0; i < offsetsIndex; i++) {
    printf("%i 0X%04X \n", i, script->offsets[i]);
  }
  script->offsetsNum = offsetsIndex;
}

void INFScriptFromBuffer(INFScript *script, uint8_t *buffer,
                         size_t bufferSize) {
  assert(buffer);
  script->originalBuffer = buffer;
  script->originalBufferSize = bufferSize;

  uint8_t chunkName[sizeof("EMC2ORDR") + 1];

  size_t readSize = 0;
  uint8_t *buff = (uint8_t *)buffer;

  ScriptChunk chunks[kCountChunkTypes];

  while (readSize < bufferSize) {
    memcpy(chunkName, buff, 4);
    chunkName[4] = '\0';
    buff += 4;
    readSize += 4;
    if (strcmp((char *)chunkName, "FORM") == 0) {
      // looks like 'FORM' is the beginning of the file.
      // size if the whole file size
      chunks[kForm]._size = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      printf("Got a FORM chunk size %u\n", chunks[kForm]._size);
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      textSize += textSize % 2 != 0 ? 1 : 0;
      buff += 4;
      readSize += 4;
      printf("Got a TEXT chunk size %u\n", textSize);
      chunks[kText]._data = buff;
      chunks[kText]._size = swap_uint32(*(uint32_t *)buff) >> 1;
      chunks[kText]._additional =
          chunks[kText]._data + (chunks[kText]._size << 1);
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "DATA") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      chunks[kData]._size = dataSize;
      chunks[kData]._data = buff;
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
        chunks[kEmc2Ordr]._size = chunkSize >> 1;
        buff += 4;
        readSize += 4;
        chunks[kEmc2Ordr]._data = buff;
        printf("Got a EMC2ORDR chunk size %u\n", chunkSize);
        buff += chunkSize;
        readSize += chunkSize;
      } else {
        printf("unknown chunk '%s'\n", chunkName);
      }
    }
  }
  decodeORDRScriptFunctions(script, chunks);
  decodeTextArea(script, chunks);
  decodeScript(script, chunks);
}
