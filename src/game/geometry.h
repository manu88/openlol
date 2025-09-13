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
