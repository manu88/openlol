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

#define UI_SCENE_EXIT_BUTTON_X MAZE_COORDS_X + 146
#define UI_SCENE_EXIT_BUTTON_Y MAZE_COORDS_Y + 110

#define UI_SCENE_EXIT_BUTTON_W 30
#define UI_SCENE_EXIT_BUTTON_H 10

#define UI_CREDITS_X 296
#define UI_CREDITS_Y 97

#define UI_MAP_BUTTON_X 290
#define UI_MAP_BUTTON_Y 30
#define UI_MAP_BUTTON_W 28
#define UI_MAP_BUTTON_H 18

#define MAP_SCREEN_EXIT_BUTTON_X 276
#define MAP_SCREEN_BUTTONS_Y 176

#define MAP_SCREEN_PREV_BUTTON_X 231
#define MAP_SCREEN_NEXT_BUTTON_X 252

#define MAP_SCREEN_PREV_NEXT_BUTTON_W 18
#define MAP_SCREEN_PREV_NEXT_BUTTON_H 18

#define MAP_SCREEN_EXIT_BUTTON_W 36
#define MAP_SCREEN_EXIT_BUTTON_H 18

#define MAP_SCREEN_NAME_X 225
#define MAP_SCREEN_NAME_Y 8

#define DIALOG_BUTTON3_X 86
#define DIALOG_BUTTON2_X 162
#define DIALOG_BUTTON1_X 240
#define DIALOG_BUTTON_Y 132

#define DIALOG_BUTTON_Y_2 167

#define DIALOG_BUTTON_W 72
#define DIALOG_BUTTON_H 10

// one character in the gang
#define CHAR_ZONE_0_1_X 167
// two characters in the gang
#define CHAR_ZONE_0_2_X 116
#define CHAR_ZONE_1_2_X 216

#define CHAR_ZONE_Y 143

#define CHAR_FACE_W 32
#define CHAR_FACE_H 32

#define CHAR_ZONE_W 66
#define CHAR_ZONE_H 34

#define GAME_MENU_X 56
#define GAME_MENU_Y 25
#define GAME_MENU_W 207
#define GAME_MENU_H 155

#define GAME_MENU_BUTTON_H 15

#define GAME_MENU_BUTTONS_START_Y 30
#define GAME_MENU_BUTTONS_START_X GAME_MENU_X + 15

#define GAME_MENU_BUTTONS_Y_OFFSET GAME_MENU_BUTTON_H + 2

#define GAME_MENU_BUTTON_W GAME_MENU_W - 30

#define GAME_MENU_RESUME_BUTTON_W 105
#define GAME_MENU_RESUME_BUTTON_X GAME_MENU_X + 87

#define CHAR_INVENTORY_HELM_X 117
#define CHAR_INVENTORY_HELM_Y 1

#define CHAR_INVENTORY_HAND1_X 117
#define CHAR_INVENTORY_HAND1_Y 25

#define CHAR_INVENTORY_ARMOR_X 117
#define CHAR_INVENTORY_ARMOR_Y 49

#define CHAR_INVENTORY_HAND2_X 117
#define CHAR_INVENTORY_HAND2_Y 73

#define CHAR_INVENTORY_SHOES_X 117
#define CHAR_INVENTORY_SHOES_Y 97

#define CHAR_INVENTORY_NECKLACE_X 192
#define CHAR_INVENTORY_NECKLACE_Y 1

#define CHAR_INVENTORY_SHIELD_X 192
#define CHAR_INVENTORY_SHIELD_Y 25

#define CHAR_INVENTORY_BRACES_X 192
#define CHAR_INVENTORY_BRACES_Y 49

#define CHAR_INVENTORY_SHIELD2_X 192
#define CHAR_INVENTORY_SHIELD2_Y 73

#define CHAR_INVENTORY_RING_LEFT_X 126
#define CHAR_INVENTORY_RING_LEFT_Y 78

#define CHAR_INVENTORY_RING_RIGHT_X 191
#define CHAR_INVENTORY_RING_RIGHT_Y 78

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

typedef enum {
  Front,
  Left,
  Back,
  Right,
} Direction;

int zoneClicked(const Point *p, int minX, int minY, int width, int height);

Orientation OrientationTurnRight(Orientation orientation);
Orientation OrientationTurnLeft(Orientation orientation);

Orientation absOrientation(Orientation partyOrientation,
                           Orientation orientation);

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
