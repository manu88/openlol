#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t tileSize;

  struct MazeBlock {
    uint8_t north;
    uint8_t south;
    uint8_t west;
    uint8_t east;
  } wallMappingIndices[1024];
} MAZEFile;

void TestCMZ(const uint8_t *buffer, size_t bufferSize);
