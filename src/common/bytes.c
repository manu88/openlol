#include "bytes.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t *readBinaryFile(const char *path, size_t *fileSize, size_t *readSize) {
  FILE *inFile = fopen(path, "rb");
  if (!inFile) {
    printf("error while reading file '%s'\n", path);
    perror("readBinaryFile.open");
    *fileSize = 0;
    *readSize = 0;
    return NULL;
  }

  fseek(inFile, 0, SEEK_END);
  long fsize = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);
  *fileSize = fsize;
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("readBinaryFile.malloc");
    fclose(inFile);
    *fileSize = 0;
    *readSize = 0;
    return NULL;
  }
  if (fread(buffer, fsize, 1, inFile) == 1) {
    *readSize = fsize;
  }
  fclose(inFile);
  return buffer;
}

int writeBinaryFile(const char *path, void *data, size_t dataSize) {
  FILE *outFile = fopen(path, "wb");
  if (!outFile) {
    perror("writeBinaryFile.open");
    return 1;
  }

  int ret = fwrite(data, dataSize, 1, outFile);
  fclose(outFile);
  return !ret;
}
