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

uint8_t *CMZDecompress(const uint8_t *inBuffer, size_t inBufferSize,
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
    uint16_t c = start[i];
    uint8_t c0 = c >> 8;
    uint8_t c1 = c & 0XFF;
    buffer[i * 2] = c1;
    buffer[i * 2 + 1] = c0;
  }
}

void TestCMZ(const uint8_t *buffer, size_t bufferSize) {
  const CMZHeader *header = (const CMZHeader *)buffer;
  printf("fileSize=%i compressionType=%i uncompressedSize=%i\n",
         header->fileSize, header->compressionType, header->uncompressedSize);

  size_t destSize = header->uncompressedSize;
  uint8_t *dest = malloc(destSize);
  assert(buffer[bufferSize - 1] == 0X80);
  const uint8_t *data = buffer + sizeof(CMZHeader);
  size_t dataSize = bufferSize - sizeof(CMZHeader);

  int bytes = LCWDecompress(data, dataSize, dest, destSize);
  assert(bytes == header->uncompressedSize);

  printf("Unkown bytes are 0X%X 0X%X 0X%X\n", dest[0], dest[1], dest[2]);
  MAZEFile *maze = (MAZEFile *)(dest + 3);
  printf("Maze w=%0X h=%0X Nof=%0X\n", maze->width, maze->height,
         maze->tileSize);
  FILE *fout = fopen("test.txt", "w");

  for (int i = 0; i < 1000; i++) {
    fprintf(fout, "%i %i %i %i\n", maze->wallMappingIndices[i].north,
            maze->wallMappingIndices[i].east, maze->wallMappingIndices[i].south,
            maze->wallMappingIndices[i].west);
  }
  fprintf(fout, "\n");
  fclose(fout);

  free(dest);
}
