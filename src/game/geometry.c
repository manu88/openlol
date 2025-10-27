#include "geometry.h"
#include <assert.h>

int zoneClicked(const Point *p, int minX, int minY, int width, int height) {
  return p->x >= minX && p->x < minX + width && p->y >= minY &&
         p->y < minY + height;
}

Orientation absOrientation(Orientation partyOrientation,
                           Orientation orientation) {
  return (orientation + partyOrientation) % 4;
}

Orientation OrientationTurnLeft(Orientation orientation) {
  orientation -= 1;
  if ((int)orientation < 0) {
    return West;
  }
  return orientation;
}

Orientation OrientationTurnRight(Orientation orientation) {
  orientation += 1;
  if (orientation > West) {
    return North;
  }
  return orientation;
}

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

Point PointGoBack(const Point *pos, Orientation orientation, int distance) {
  Point pt = *pos;
  switch (orientation) {
  case North:
    pt.y += distance;
    break;
  case East:
    pt.x -= distance;
    break;
  case South:
    pt.y -= distance;
    break;
  case West:
    pt.x += distance;
    break;
  }
  return pt;
}

Point PointGoRight(const Point *pos, Orientation orientation, int distance) {

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

Point PointGoLeft(const Point *pos, Orientation orientation, int distance) {
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
    p = PointGoLeft(&p, orientation, leftDist);
  } else if (leftDist < 0) {
    p = PointGoRight(&p, orientation, -leftDist);
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

void GetGameCoords(uint16_t x, uint16_t y, uint16_t *xOut, uint16_t *yOut) {
  assert(xOut);
  assert(yOut);
  *xOut = x << 8;
  *yOut = (y + 0XFF00) << 8;
}
