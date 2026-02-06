#pragma once
#include "formats/format_cps.h"
#include "formats/format_fnt.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_wsa.h"
#include "geometry.h"
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
  SDL_Texture *pixBuf;
  SDL_Renderer *renderer;
  SDL_Window *window;
  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  SDL_Cursor *cursor;

  FNTHandle defaultFont;
  FNTHandle font6p;

  SHPHandle itemShapes;
  SHPHandle charFaces[NUM_CHARACTERS];
  SHPHandle automapShapes;
  SHPHandle gameShapes;
  CPSImage inventoryBackgrounds[INVENTORY_TYPES_NUM];
  CPSImage playField;
  CPSImage mapBackground;
  CPSImage gameTitle;
  CPSImage loadedbitMap;

  char *dialogTextBuffer;
  char *dialogText; // will either be NULL or pointing to dialogTextBuffer

  uint8_t *defaultPalette;

  int showBitmap;
  int dialogBoxFrames;
  int showBigDialog;
  int drawExitSceneButton;
  int exitSceneButtonDisabled;
  char *buttonText[3];
} Display;

int DisplayInit(Display *display);
void DisplayRelease(Display *display);

void DisplayUpdate(Display *display);

void DisplayRenderCPS(Display *display, const CPSImage *image, int w, int h);
void DisplayRenderCPSPart(Display *display, const CPSImage *image, int destX,
                          int destY, int sourceX, int sourceY, int imageW,
                          int imageH, int sourceImageWidth);
void DisplayRenderCPSAt(Display *display, const CPSImage *image, int destX,
                        int destY, int sourceW, int sourceH, int imageW,
                        int imageH);
void DisplayRenderSHP(Display *display, const SHPFrame *frame, int xPos,
                      int yPos, const uint8_t *palette);

void DisplayRenderBitmap(Display *display, const uint8_t *imgData,
                         size_t dataSize, const uint8_t *paletteBuffer,
                         int destX, int destY, int sourceW, int sourceH,
                         int imageW, int imageH);
void DisplayRenderWSA(Display *display, const uint8_t *imgData,
                      const WSAHandle *wsa, int destX, int destY);

void DisplayResetDialog(Display *display);
int DisplayActiveDelay(Display *display, int tickLength);
int DisplayWaitMouseEvent(Display *display, SDL_Event *event, int tickLength);
void showBigDialogZone(Display *display);

void DisplayLoadBackgroundInventoryIfNeeded(Display *display, int charId);

void DisplayDoSceneFade(Display *display, int numFrames, int tickLength);
void DisplayDoScreenFade(Display *display, int numFrames, int tickLength);
void DisplayDrawDisabledOverlay(Display *display, int x, int y, int w, int h);

void DisplayExpandDialogBox(Display *display, int tickLength);
void DisplayShrinkDialogBox(Display *display, int tickLength);

int DisplayWaitForClickOrKey(Display *display, int tickLength);

void DisplayCreateCursorForItem(Display *display, uint16_t frameId);

void DisplayClearDialogButtons(Display *display);
