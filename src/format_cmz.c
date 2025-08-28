#include "format_cmz.h"
#include "format_lcw.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
} CMZHeader;

void TestCMZ(const uint8_t *buffer, size_t bufferSize) {
  const CMZHeader *header = (const CMZHeader *)buffer;
  printf("fileSize=%i compressionType=%i uncompressedSize=%i\n",
         header->fileSize, header->compressionType, header->uncompressedSize);

  size_t destSize = header->uncompressedSize;
  uint8_t *dest = malloc(destSize);

  LCWDecompress(buffer + sizeof(CMZHeader), bufferSize - sizeof(CMZHeader),
                dest, destSize);
  printf("LCWDecompress wrote %zi bytes\n", destSize);

  MAZEFile *maze = (MAZEFile *)dest;
  printf("maze %i %i %i\n", maze->width, maze->height, maze->tileSize);

  FILE *fout = fopen("test.txt", "w");

  for (int i = 0; i < 1024; i++) {
    printf("%i: n=%i s=%i e=%i w=%i\n", i, maze->wallMappingIndices[i].north,
           maze->wallMappingIndices[i].south, maze->wallMappingIndices[i].east,
           maze->wallMappingIndices[i].west);
    if (maze->wallMappingIndices[i].north == 0 &&
        maze->wallMappingIndices[i].south == 0 &&
        maze->wallMappingIndices[i].east == 0 &&
        maze->wallMappingIndices[i].west == 0) {
      fprintf(fout, " ");
    } else {
      fprintf(fout, "0");
    }
    if (i % 23 == 0) {
      fprintf(fout, "\n");
    }
  }
  fprintf(fout, "\n");
  fclose(fout);
  free(dest);
}
