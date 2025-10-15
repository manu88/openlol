#pragma once
#include <stddef.h>
#include <stdint.h>

#define MAZE_COORDS_X (int)112
#define MAZE_COORDS_Y (int)0
#define MAZE_COORDS_W (int)176
#define MAZE_COORDS_H (int)120

#define INVENTORY_SCREEN_X MAZE_COORDS_X
#define INVENTORY_SCREEN_Y MAZE_COORDS_Y
#define INVENTORY_SCREEN_W (int)208
#define INVENTORY_SCREEN_H MAZE_COORDS_H

#define INVENTORY_SCREEN_EXIT_BUTTON_X MAZE_COORDS_X + (int)162
#define INVENTORY_SCREEN_EXIT_BUTTON_Y MAZE_COORDS_Y + (int)100
#define INVENTORY_SCREEN_EXIT_BUTTON_W (int)39
#define INVENTORY_SCREEN_EXIT_BUTTON_H (int)13

#define DIALOG_BOX_X 83
#define DIALOG_BOX_Y 122
#define DIALOG_BOX_W 235
#define DIALOG_BOX_H 20

#define DIALOG_BOX_H2 56

#define UI_DIR_BUTTON_W 21
#define UI_DIR_BUTTON_H 18

#define UI_MENU_INV_BUTTON_W 21
#define UI_MENU_INV_BUTTON_H 21

#define UI_TURN_LEFT_BUTTON_X 11
#define UI_TURN_LEFT_BUTTON_Y 131

#define UI_MENU_BUTTON_X 11
#define UI_MENU_BUTTON_Y 179

#define UI_INVENTORY_BUTTON_X 85
#define UI_INVENTORY_BUTTON_Y 179

// one character in the gang
#define CHAR_FACE_0_1_X 167
#define CHAR_FACE_Y 144

#define CHAR_FACE_W 32
#define CHAR_FACE_H 32

// two characters in the gang
#define CHAR_FACE_0_2_X 116
#define CHAR_FACE_1_2_X 216

typedef struct {
  uint16_t x;
  uint16_t y;
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

Orientation OrientationTurnRight(Orientation orientation);
Orientation OrientationTurnLeft(Orientation orientation);

Point PointGoFront(const Point *pos, Orientation orientation, int distance);
Point PointGoLeft(const Point *pos, Orientation orientation, int distance);
Point PointGoRight(const Point *pos, Orientation orientation, int distance);
Point PointGoBack(const Point *pos, Orientation orientation, int distance);
Point PointGo(const Point *pos, Orientation orientation, int frontDist,
              int leftDist);

void BlockGetCoordinates(uint16_t *x, uint16_t *y, int block, uint16_t xOffs,
                         uint16_t yOffs);
uint16_t BlockFromCoords(uint16_t x, uint16_t y);
uint16_t BlockCalcNewPosition(uint16_t curBlock, uint16_t direction);

void GetRealCoords(uint16_t x, uint16_t y, uint16_t *xOut, uint16_t *yOut);
void GetGameCoords(uint16_t x, uint16_t y, uint16_t *xOut, uint16_t *yOut);
void GetGameCoordsFromBlock(uint16_t block, uint16_t *xOut, uint16_t *yOut);
