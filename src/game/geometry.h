#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  Point coords;
  uint8_t valid;
} ViewConeEntry;

#define VIEW_CONE_NUM_CELLS 17

typedef enum {
  North = 0,
  East = 1,
  South = 2,
  West = 3,
} Orientation;

Point PointGoFront(const Point *pos, Orientation orientation, int distance);
Point PointGoLeft(const Point *pos, Orientation orientation, int distance);
Point PointGoRight(const Point *pos, Orientation orientation, int distance);
Point PointGo(const Point *pos, Orientation orientation, int frontDist,
              int leftDist);

#define MAZE_COORDS_X (int)112
#define MAZE_COORDS_Y (int)0
#define MAZE_COORDS_W (int)174
#define MAZE_COORDS_H (int)120

void BlockGetCoordinates(uint16_t *x, uint16_t *y, int block, uint16_t xOffs,
                         uint16_t yOffs);
uint16_t BlockFromCoords(uint16_t x, uint16_t y);
uint16_t BlockCalcNewPosition(uint16_t curBlock, uint16_t direction);

void GetRealCoords(uint16_t x, uint16_t y, uint16_t*xOut, uint16_t *yOut);
