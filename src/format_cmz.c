#include "format_cmz.h"
#include "bytes.h"
#include "format_cps.h"
#include "format_lcw.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
} CMZHeader;

uint8_t *CMZ_Uncompress(const uint8_t *inBuffer, size_t inBufferSize,
                        size_t *outBufferSize) {
  assert(inBuffer);
  assert(outBufferSize);
  const CMZHeader *header = (const CMZHeader *)inBuffer;

  *outBufferSize = header->uncompressedSize;
  uint8_t *dest = malloc(*outBufferSize);
  LCWDecompress(inBuffer + sizeof(CMZHeader), -sizeof(CMZHeader), dest,
                *outBufferSize);
  return dest;
}

static void swapBuffer(uint8_t *buffer, size_t bufferSize) {
  // assert(bufferSize % 2 == 0);
  uint16_t *start = (uint16_t *)buffer;
  size_t size = bufferSize / 2;
  for (int i = 0; i < size; i++) {
    uint16_t c = swap_uint16(start[i]);
    uint8_t c0 = c >> 8;
    uint8_t c1 = c & 0XFF;
    buffer[i * 2] = c0;
    buffer[i * 2 + 1] = c1;
  }
}

void TestCMZ(const uint8_t *buffer, size_t bufferSize) {
  const CMZHeader *header = (const CMZHeader *)buffer;
  printf("fileSize=%i compressionType=%i uncompressedSize=%i\n",
         header->fileSize, header->compressionType, header->uncompressedSize);

  size_t destSize = header->uncompressedSize;
  uint8_t *dest = malloc(destSize);
  assert(buffer[bufferSize - 1] == 0X80);
  LCWDecompress(buffer + sizeof(CMZHeader), bufferSize - sizeof(CMZHeader),
                dest, destSize);
  printf("LCWDecompress wrote %zi bytes\n", destSize);

  MAZEFile *maze = (MAZEFile *)(dest);
  FILE *fout = fopen("test.txt", "w");

  for (int i = 0; i < 1000; i++) {
    fprintf(fout, "%i %i %i %i\n", maze->wallMappingIndices[i].north,
            maze->wallMappingIndices[i].south, maze->wallMappingIndices[i].east,
            maze->wallMappingIndices[i].west);
  }
  fprintf(fout, "\n");
  fclose(fout);

  free(dest);
}
