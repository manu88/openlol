#pragma once
#include "formats/format_cps.h"
#include "formats/format_fnt.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "geometry.h"
#include "menu.h"
#include <SDL2/SDL.h>
#include <stdint.h>

// FIXME: duplicate in game_ctx.h
#ifndef INVENTORY_TYPES_NUM
#define INVENTORY_TYPES_NUM 7
#endif

typedef struct {
  Point pos;
  uint8_t pending;
  uint8_t isRightClick;
} MouseEvent;

typedef struct {
  MouseEvent mouseEv;
  SDL_Texture *pixBuf;
  SDL_Renderer *renderer;
  SDL_Window *window;
  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];
  SHPHandle itemShapes;
  SHPHandle charFaces[NUM_CHARACTERS];
  SHPHandle automapShapes;
  SHPHandle gameShapes;
  CPSImage inventoryBackgrounds[INVENTORY_TYPES_NUM];
  CPSImage playField;
  CPSImage loadedbitMap;
  int showBitmap;
  CPSImage mapBackground;
  CPSImage gameTitle;
  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  char *dialogTextBuffer;
  char *dialogText; // will either be NULL or pointing to dialogTextBuffer
  FNTHandle defaultFont;
  FNTHandle font6p;
  uint8_t *defaultPalette;
  int controlDisabled;
  SDL_Cursor *cursor;
  Menu *currentMenu;
  int shouldUpdate;

  int dialogBoxFrames;
  int showBigDialog;
  int drawExitSceneButton;
  int exitSceneButtonDisabled;
  char *buttonText[3];
} Display;

int DisplayInit(Display *display);
void DisplayRelease(Display *display);

void DisplayRender(Display *display);

void DisplayLoadBackgroundInventoryIfNeeded(Display *display, int charId);

void DisplayDoSceneFade(Display *display, int numFrames, int tickLength);
