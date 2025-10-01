#include "geometry.h"
#include <assert.h>

Point PointGoFront(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.y -= distance;
    break;
  case East:
    pt.x += distance;
    break;
  case South:
    pt.y += distance;
    break;
  case West:
    pt.x -= distance;
    break;
  }
  return pt;
}

Point PointGoLeft(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.x += distance;
    break;
  case East:
    pt.y += distance;
    break;
  case South:
    pt.x -= distance;
    break;
  case West:
    pt.y -= distance;
    break;
  }
  return pt;
}

Point PointGoRight(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.x -= distance;
    break;
  case East:
    pt.y -= distance;
    break;
  case South:
    pt.x += distance;
    break;
  case West:
    pt.y += distance;
    break;
  }
  return pt;
}

Point PointGo(const Point *pos, Orientation orientation, int frontDist,
              int leftDist) {
  Point p = *pos;
  if (frontDist) {
    p = PointGoFront(&p, orientation, frontDist);
  }
  if (leftDist > 0) {
    p = PointGoRight(&p, orientation, leftDist);
  } else if (leftDist < 0) {
    p = PointGoLeft(&p, orientation, -leftDist);
  }
  return p;
}

void BlockGetCoordinates(uint16_t *x, uint16_t *y, int block, uint16_t xOffs,
                         uint16_t yOffs) {
  *x = (block & 0x1F) << 8 | xOffs;
  *y = ((block & 0xFFE0) << 3) | yOffs;
}

uint16_t BlockFromCoords(uint16_t x, uint16_t y) {
  return (((y & 0xFF00) >> 3) | (x >> 8)) & 0x3FF;
}

uint16_t BlockCalcNewPosition(uint16_t curBlock, uint16_t direction) {
  static const int16_t blockPosTable[] = {-32, 1, 32, -1};
  assert(direction < sizeof(blockPosTable) / sizeof(int16_t));
  return (curBlock + blockPosTable[direction]) & 0x3FF;
}

void GetRealCoords(uint16_t x, uint16_t y, uint16_t *xOut, uint16_t *yOut) {
  assert(xOut);
  assert(yOut);
  *xOut = x >> 8;
  *yOut = (y & 0XFF00) >> 8;
}

void GetGameCoordsFromBlock(uint16_t block, uint16_t *xOut, uint16_t *yOut) {
  uint16_t x = 0;
  uint16_t y = 0;
  BlockGetCoordinates(&x, &y, block, 0x80, 0x80);
  GetRealCoords(x, y, xOut, yOut);
}

void GetGameCoords(uint16_t x, uint16_t y, uint16_t *xOut, uint16_t *yOut) {
  assert(xOut);
  assert(yOut);
  *xOut = x << 8;
  *yOut = (y + 0XFF00) << 8;
}
