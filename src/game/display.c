#include "display.h"
#include "SDL_events.h"
#include "SDL_timer.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "renderer.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int initSDL(Display *display) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }

  display->window =
      SDL_CreateWindow("Lands Of Lore", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, PIX_BUF_WIDTH * SCREEN_FACTOR,
                       PIX_BUF_HEIGHT * SCREEN_FACTOR, SDL_WINDOW_SHOWN);
  if (!display->window) {
    printf("Window could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }
  display->renderer =
      SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED);
  if (!display->renderer) {
    printf("Renderer could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }

  display->pixBuf = SDL_CreateTexture(
      display->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (display->pixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
    return 0;
  }
  return 1;
}

int DisplayInit(Display *display) {
  memset(display, 0, sizeof(Display));

  if (!initSDL(display)) {
    return 0;
  }
  display->shouldUpdate = 1;

  display->dialogTextBuffer = malloc(DIALOG_BUFFER_SIZE);
  assert(display->dialogTextBuffer);

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "TITLE.CPS"));

    if (CPSImageFromBuffer(&display->gameTitle, f.buffer, f.bufferSize) == 0) {
      printf("unable to get Title.CPS Data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PARCH.CPS"));

    if (CPSImageFromBuffer(&display->mapBackground, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get mapBackground Data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PLAYFLD.CPS"));

    if (CPSImageFromBuffer(&display->playField, f.buffer, f.bufferSize) == 0) {
      printf("unable to get playFieldData\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFile(&f, "ITEMICN.SHP"));
    if (SHPHandleFromCompressedBuffer(&display->itemShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get ITEMICN.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "GAMESHP.SHP"));
    if (SHPHandleFromCompressedBuffer(&display->gameShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get GAMESHP.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "AUTOBUT.SHP"));
    if (SHPHandleFromCompressedBuffer(&display->automapShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get AUTOBUT.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT9PN.FNT"));
    if (FNTHandleFromBuffer(&display->defaultFont, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get FONT9P.FNT data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT6P.FNT"));

    if (FNTHandleFromBuffer(&display->font6p, f.buffer, f.bufferSize) == 0) {
      printf("unable to get FONT6P.FNT data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, "GERIM.CPS", "O01A.PAK"));
    CPSImage img = {0};
    assert(CPSImageFromBuffer(&img, f.buffer, f.bufferSize));
    display->defaultPalette = malloc(img.paletteSize);
    memcpy(display->defaultPalette, img.palette, img.paletteSize);
    CPSImageRelease(&img);
  }

  return 1;
}

void DisplayRelease(Display *display) {
  SDL_DestroyRenderer(display->renderer);
  SDL_DestroyWindow(display->window);
  SDL_DestroyTexture(display->pixBuf);
  CPSImageRelease(&display->playField);
  CPSImageRelease(&display->gameTitle);
  CPSImageRelease(&display->mapBackground);
  SHPHandleRelease(&display->automapShapes);
  SHPHandleRelease(&display->gameShapes);
  free(display->dialogTextBuffer);
  SDL_FreeCursor(display->cursor);
  DisplayClearDialogButtons(display);

  for (int i = 0; i < INVENTORY_TYPES_NUM; i++) {
    if (display->inventoryBackgrounds[i].data) {
      CPSImageRelease(&display->inventoryBackgrounds[i]);
    }
  }
  free(display->defaultPalette);
}

void DisplayLoadBackgroundInventoryIfNeeded(Display *display, int charId) {
  int invId = inventoryTypeForId[charId];
  if (display->inventoryBackgrounds[invId].data == NULL) {
    GameFile f = {0};
    char name[12] = "";
    assert(invId > 0 && invId < 7);
    snprintf(name, 12, "INVENT%1i.CPS", invId);
    assert(GameEnvironmentGetGeneralFile(&f, name));
    assert(CPSImageFromBuffer(&display->inventoryBackgrounds[invId], f.buffer,
                              f.bufferSize));
  }
}

void DisplayResetDialog(Display *display) { display->dialogText = NULL; }

static void dimRect(SDL_Texture *texture, int startX, int startY, int w,
                    int h) {
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      uint8_t r = ((row[x] & 0X00FF0000) >> 16) / 1.5;
      uint8_t g = ((row[x] & 0X0000FF00) >> 8) / 1.5;
      uint8_t b = ((row[x] & 0X000000FF)) / 1.5;
      row[x] = 0XFF000000 + (r << 0X10) + (g << 0X8) + b;
    }
  }
  SDL_UnlockTexture(texture);
}

void DisplayRender(Display *display) {
  SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                   PIX_BUF_HEIGHT * SCREEN_FACTOR};
  assert(SDL_RenderCopy(display->renderer, display->pixBuf, NULL, &dest) == 0);
  SDL_RenderPresent(display->renderer);
}

int DisplayWaitMouseEvent(Display *display, SDL_Event *event, int tickLength) {
  uint64_t time = SDL_GetTicks64();
  while (1) {
    SDL_WaitEventTimeout(event, tickLength);
    if (event->type == SDL_QUIT) {
      return 0;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
      return 1;
    }
    uint64_t elapsed = SDL_GetTicks64() - time;
    if (elapsed >= tickLength) {
      break;
    }
  }
  return -1;
}

int DisplayActiveDelay(Display *display, int tickLength) {
  uint64_t time = SDL_GetTicks64();
  while (1) {
    SDL_Event event = {0};
    SDL_WaitEventTimeout(&event, tickLength);
    if (event.type == SDL_QUIT) {
      return 0;
    }
    uint64_t elapsed = SDL_GetTicks64() - time;
    if (elapsed >= tickLength) {
      return 1;
    }
  }
  return 1;
}

void DisplayDoSceneFade(Display *display, int numFrames, int tickLength) {
  while (numFrames--) {
    if (DisplayActiveDelay(display, tickLength) == 0) {
      return;
    }
    dimRect(display->pixBuf, MAZE_COORDS_X, MAZE_COORDS_Y, MAZE_COORDS_W,
            MAZE_COORDS_H);
    DisplayRender(display);
  }
}

void DisplayDrawDisabledOverlay(Display *display, int x, int y, int w, int h) {
  void *data;
  int pitch;
  SDL_Rect r = {.x = x, .y = y, .w = w, .h = h};
  SDL_LockTexture(display->pixBuf, &r, &data, &pitch);
  for (int x = 0; x < r.w; x++) {
    for (int y = 0; y < r.h; y++) {
      if (x % 2 == 0 && y % 2 == 0) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      } else if (x % 2 == 1 && y % 2 == 1) {
        drawPix(data, pitch, 0, 0, 0, x, y);
      }
    }
  }
  SDL_UnlockTexture(display->pixBuf);
}

static void animateDialogZoneOnce(Display *display, void *data, int pitch) {
  // copy outline
  int offset = display->dialogBoxFrames;
  const int size = 20;
  for (int y = DIALOG_BOX_H - size; y < DIALOG_BOX_H; y++) {
    const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
    uint32_t *rowDest = (unsigned int *)((char *)data + pitch * (y + offset));
    for (int x = 0; x < DIALOG_BOX_W; x++) {
      rowDest[x] = rowSource[x];
    }
  }

  if (display->dialogBoxFrames >= size) {
    // need to copy the dialog background
    int size = DIALOG_BOX_H - 1;
    if (display->dialogBoxFrames == DIALOG_BOX_H2 - DIALOG_BOX_H) {
      size++;
    }
    for (int y = 1; y < size; y++) {
      const uint32_t *rowSource = (unsigned int *)((char *)data + pitch * y);
      uint32_t *rowDest =
          (unsigned int *)((char *)data + pitch * (y + (DIALOG_BOX_H - 2)));
      for (int x = 0; x < DIALOG_BOX_W; x++) {
        rowDest[x] = rowSource[x];
      }
    }
  }
}

static int renderExpandDialogBox(Display *display) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(display->pixBuf, &rect, &data, &pitch);
  animateDialogZoneOnce(display, data, pitch);
  SDL_UnlockTexture(display->pixBuf);

  if (display->dialogBoxFrames <= DIALOG_BOX_H2 - DIALOG_BOX_H) {
    display->dialogBoxFrames += 1;
    return 0;
  }
  return 1;
}

void DisplayExpandDialogBox(Display *display, int tickLength) {
  int ret = 0;
  do {
    ret = renderExpandDialogBox(display);
    if (DisplayActiveDelay(display, tickLength) == 0) {
      return;
    }
    SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                     PIX_BUF_HEIGHT * SCREEN_FACTOR};
    assert(SDL_RenderCopy(display->renderer, display->pixBuf, NULL, &dest) ==
           0);
    SDL_RenderPresent(display->renderer);
  } while (!ret);

  display->showBigDialog = 1;
}

void showBigDialogZone(Display *display) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(display->pixBuf, &rect, &data, &pitch);
  for (int i = 0; i < 1 + DIALOG_BOX_H2 - DIALOG_BOX_H; i++) {
    animateDialogZoneOnce(display, data, pitch);
  }
  SDL_UnlockTexture(display->pixBuf);
}

static int renderShrinkDialogBox(Display *display) {
  void *data;
  int pitch;
  SDL_Rect rect = {DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W, DIALOG_BOX_H2};
  SDL_LockTexture(display->pixBuf, &rect, &data, &pitch);
  for (int i = 0; i < 1 + DIALOG_BOX_H2 + display->dialogBoxFrames; i++) {
    animateDialogZoneOnce(display, data, pitch);
  }
  SDL_UnlockTexture(display->pixBuf);

  if (display->dialogBoxFrames > 0) {
    display->dialogBoxFrames -= 1;
    return 0;
  }
  return 1;
}

void DisplayShrinkDialogBox(Display *display, int tickLength) {
  int ret = 0;
  do {
    ret = renderShrinkDialogBox(display);
    if (DisplayActiveDelay(display, tickLength) == 0) {
      return;
    }
    SDL_Rect dest = {0, 0, PIX_BUF_WIDTH * SCREEN_FACTOR,
                     PIX_BUF_HEIGHT * SCREEN_FACTOR};
    assert(SDL_RenderCopy(display->renderer, display->pixBuf, NULL, &dest) ==
           0);
    SDL_RenderPresent(display->renderer);
  } while (!ret);

  display->showBigDialog = 0;
}

int DisplayWaitForClickOrKey(Display *display, int tickLength) {
  DisplayRender(display);
  while (1) {
    SDL_Event event = {0};
    SDL_WaitEventTimeout(&event, tickLength);
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_KEYDOWN ||
        event.type == SDL_QUIT) {
      return 1;
    }
  }
  assert(0);
}

void DisplayCreateCursorForItem(Display *display, uint16_t frameId) {
  SDL_Cursor *prevCursor = display->cursor;

  const int w = 20 * SCREEN_FACTOR;
  SDL_Surface *s = SDL_CreateRGBSurface(0, w, w, 32, 0, 0, 0, 0);
  assert(s);
  SDL_Renderer *r = SDL_CreateSoftwareRenderer(s);
  assert(r);
  SHPFrame frame = {0};
  SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, 127, 127, 127));
  assert(SHPHandleGetFrame(&display->itemShapes, &frame, frameId));
  assert(SHPFrameGetImageData(&frame));

  SDL_RenderClear(r);
  SDL_SetRenderDrawColor(r, 127, 127, 127, 255);
  SDL_RenderFillRect(r, NULL);
  drawSHPFrameCursor(r, &frame, 0, 0, display->defaultPalette);
  display->cursor = SDL_CreateColorCursor(s, frameId == 0 ? 0 : w / 2,
                                          frameId == 0 ? 0 : w / 2);
  SDL_SetCursor(display->cursor);
  SDL_DestroyRenderer(r);
  SDL_FreeSurface(s);
  SHPFrameRelease(&frame);
  if (prevCursor) {
    SDL_FreeCursor(prevCursor);
  }
}

void DisplayClearDialogButtons(Display *display) {
  for (int i = 0; i < 3; i++) {
    if (display->buttonText[i]) {
      free(display->buttonText[i]);
      display->buttonText[i] = NULL;
    }
  }
}
