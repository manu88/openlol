#include "geometry.h"

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
