#pragma once
#include <stddef.h>
#include <stdint.h>

#define MAZE_NUM_CELL (size_t)1024

typedef struct {
  uint8_t face[4];
} MazeBlock;

typedef struct {
  int16_t width;
  int16_t height;
  int16_t tileSize;

  MazeBlock wallMappingIndices[MAZE_NUM_CELL];
} Maze;

typedef struct {
  Maze *maze;
} MazeHandle;

uint8_t *CMZDecompress(const uint8_t *inBuffer, size_t inBufferSize,
                       size_t *outBufferSize);

void MazeHandleRelease(MazeHandle *mazeHandle);
int MazeHandleFromBuffer(MazeHandle *mazeHandle, const uint8_t *inBuffer,
                         size_t inBufferSize);
