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
void INFScriptRelease(INFScript *script) {
  free(script->originalBuffer);
  free(script->segments);
}

void INFScriptListText(const INFScript *script) {
  printf("TEXT chunk:\n");
  // first is size
  for (uint32_t pos = 0; pos < (script->chunks[kText]._size << 1); ++pos) {
    if (swap_uint16(((uint16_t *)script->chunks[kText]._data)[pos]) >=
        script->originalBufferSize) {
      break;
    }
    uint32_t startoffset =
        swap_uint16(((uint16_t *)script->chunks[kText]._data)[pos]);
    printf("\tIndex: %X Offset: %X:\n", pos, startoffset);
    /*uint32_t endoffset = TO_BE_16(((uint16_t*)_chunks[kText]._data)[pos+1]);
    printf("\nstartoffset = %d, endoffset = %d\n\n", startoffset, endoffset);
    for (; startoffset < endoffset; ++startoffset) {
            printf("%c", *(char*)(_chunks[kText]._additional + startoffset));
    }
    printf("\n");*/
    printf("%d(%d) : %s\n", pos, startoffset,
           (char *)(script->chunks[kText]._data + startoffset));
  }
}

size_t INFScriptGetCodeBinarySize(const INFScript *script) {
  return script->chunks[kData]._size / 2;
}

const uint16_t *INFScriptGetCodeBinary(const INFScript *script) {
  return (uint16_t *)script->chunks[kData]._data;
}

static uint16_t getNextOffset(uint16_t offset, const ScriptChunk *chunks) {
  const uint16_t *b = (const uint16_t *)chunks[kEmc2Ordr]._data;
  uint16_t minOffset = 0XFFFF;
  for (int i = 0; i < chunks[kEmc2Ordr]._size; i++) {
    if (b[i] == 0XFFFF) {
      continue;
    }
    uint16_t v = swap_uint16(b[i]);
    if (v > offset && v < minOffset) {
      minOffset = v;
    }
  }
  return minOffset;
}

uint16_t *INFScriptGetBlock(const INFScript *script, int block,
                            size_t *numInstructions) {
  assert(numInstructions);
  if (block >= script->segmentsNum) {
    return NULL;
  }
  *numInstructions =
      script->segments[block].end - script->segments[block].start;
  return (uint16_t *)script->chunks[kData]._data +
         script->segments[block].start;
}

void INFScriptListScriptFunctions(const INFScript *script) {
  printf("Got %zu offsets\n", script->segmentsNum);
  for (int i = 0; i < script->segmentsNum; i++) {
    printf("%i 0X%04X 0X%04X len=%i\n", i, script->segments[i].start,
           script->segments[i].end,
           script->segments[i].end - script->segments[i].start);
  }
}

static int createSegments(INFScript *script) {
  if (script->segments == NULL) {
    script->segments = malloc(128 * sizeof(ScriptSegment));
    int offsetsIndex = 0;
    const uint16_t *b = (const uint16_t *)script->chunks[kEmc2Ordr]._data;
    for (int i = 0; i < script->chunks[kEmc2Ordr]._size; i++) {
      if (b[i] != 0XFFFF) {
        uint16_t offset = swap_uint16(b[i]);
        assert(offsetsIndex < 128);
        script->segments[offsetsIndex++].start = offset;
      }
    }
    script->segmentsNum = offsetsIndex;
    // FIXME: do this with one loop
    for (int i = 0; i < script->segmentsNum; i++) {
      uint16_t nextOffset =
          getNextOffset(script->segments[i].start, script->chunks);
      script->segments[i].end = nextOffset;
    }
  }
  return 1;
}

int INFScriptFromBuffer(INFScript *script, uint8_t *buffer, size_t bufferSize) {
  assert(buffer);
  script->originalBuffer = buffer;
  script->originalBufferSize = bufferSize;

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
      script->chunks[kForm]._size = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      // printf("Got a FORM chunk size %u\n", chunks[kForm]._size);
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      textSize += textSize % 2 != 0 ? 1 : 0;
      buff += 4;
      readSize += 4;
      // printf("Got a TEXT chunk size %u\n", textSize);
      script->chunks[kText]._data = buff;
      script->chunks[kText]._size = swap_uint32(*(uint32_t *)buff) >> 1;
      script->chunks[kText]._additional =
          script->chunks[kText]._data + (script->chunks[kText]._size << 1);
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "DATA") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      script->chunks[kData]._size = dataSize;
      script->chunks[kData]._data = buff;
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
        script->chunks[kEmc2Ordr]._size = chunkSize / 2;
        buff += 4;
        readSize += 4;
        script->chunks[kEmc2Ordr]._data = buff;
        // printf("Got a EMC2ORDR chunk size %u\n", chunkSize);
        buff += chunkSize;
        readSize += chunkSize;
      } else {
        printf("unknown chunk '%s'\n", chunkName);
        assert(0);
      }
    }
  }
  // decodeORDRScriptFunctions(script);
  // decodeTextArea(script);
  // decodeScriptSegments(script);
  // decodeScript(script);
  return createSegments(script);
}
