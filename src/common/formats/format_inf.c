#include "format_inf.h"
#include "bytes.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Adapted from
// https://github.com/scummvm/scummvm-tools/blob/cd2a721f48460b7c8f69024dbe5f26971491599c/dekyra.cpp
// https://github.com/OpenDUNE/OpenDUNE/blob/master/src/script/script.c

void INFScriptInit(INFScript *script) { memset(script, 0, sizeof(INFScript)); }
void INFScriptRelease(INFScript *script) {}

int INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize) {
  assert(buffer);

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
      buff += 4;
      readSize += 4;
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      textSize += textSize % 2 != 0 ? 1 : 0;
      script->textSize = textSize;
      buff += 4;
      readSize += 4;
      script->text = buff;
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "DATA") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      script->dataSize = dataSize;
      script->data = (uint16_t *)buff;
      // printf("Got a DATA chunk size %u\n", dataSize);
      //  mostly it will be the end of the file because all files should end
      //  with a 'DATA' chunk
      buff += dataSize;
      readSize += dataSize;
    } else {
      memcpy(&chunkName[4], buff, 4);
      chunkName[8] = '\0';
      buff += 4;
      readSize += 4;
      if (strcmp((char *)chunkName, "EMC2ORDR") == 0) {
        uint32_t chunkSize = swap_uint32(*(uint32_t *)buff);
        script->ordrSize = chunkSize / 2;
        buff += 4;
        readSize += 4;
        script->ordr = (uint16_t *)buff;

        // printf("Got a EMC2ORDR chunk size %u\n", chunkSize);
        buff += chunkSize;
        readSize += chunkSize;
      } else {
        printf("unknown chunk '%s'\n", chunkName);
        assert(0);
      }
    }
  }

  if (script->textSize > 0) {
    assert(script->text);
    int i = 0;
    for (; i < script->textSize / 2; i++) {
      uint16_t *b = (uint16_t *)script->text;
      uint16_t offset = swap_uint16(b[i]);
      if (offset >= script->textSize) {
        break;
      }
    }
    script->numTextStrings = i;
  }
  return 1;
}

int INFScriptGetNumFunctions(const INFScript *script) {
  return script->ordrSize;
}

int INFScriptGetFunctionOffset(const INFScript *script, uint16_t functionNum) {
  assert(functionNum < INFScriptGetNumFunctions(script));
  if (script->ordr[functionNum] != 0XFFFF) {
    return swap_uint16(script->ordr[functionNum]);
  }
  return -1;
}

const char *INFScriptGetDataString(const INFScript *script, int16_t index) {
  assert(index <= script->numTextStrings);
  if (script->textSize == 0) {
    return NULL;
  }
  uint16_t offset = swap_uint16(((uint16_t *)script->text)[index]);
  return (const char *)script->text + offset;
}
