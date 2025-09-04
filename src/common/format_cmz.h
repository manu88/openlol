#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int16_t width;
  int16_t height;
  int16_t tileSize;

  struct MazeBlock {
    uint8_t north;
    uint8_t east;
    uint8_t south;
    uint8_t west;
  } wallMappingIndices[1024];
} MAZEFile;

uint8_t *CMZDecompress(const uint8_t *inBuffer, size_t inBufferSize,
                       size_t *outBufferSize);

void TestCMZ(const uint8_t *buffer, size_t bufferSize);
