#include "display.h"
#include "game_ctx.h"
#include "game_envir.h"
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

void DisplayRelease(Display *renderer) {
  SDL_DestroyRenderer(renderer->renderer);
  SDL_DestroyWindow(renderer->window);
  SDL_DestroyTexture(renderer->pixBuf);
  CPSImageRelease(&renderer->playField);
  CPSImageRelease(&renderer->gameTitle);
  CPSImageRelease(&renderer->mapBackground);
  SHPHandleRelease(&renderer->automapShapes);
  SHPHandleRelease(&renderer->gameShapes);
  free(renderer->dialogTextBuffer);
  SDL_FreeCursor(renderer->cursor);

  for (int i = 0; i < INVENTORY_TYPES_NUM; i++) {
    if (renderer->inventoryBackgrounds[i].data) {
      CPSImageRelease(&renderer->inventoryBackgrounds[i]);
    }
  }
  free(renderer->defaultPalette);
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

void DimRect(SDL_Texture *texture, int startX, int startY, int w, int h) {
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

void DisplayDoSceneFade(Display *display, int numFrames, int tickLength) {
  while (numFrames--) {
    SDL_Delay(tickLength);
    SDL_PollEvent(NULL);
    DimRect(display->pixBuf, MAZE_COORDS_X, MAZE_COORDS_Y, MAZE_COORDS_W,
            MAZE_COORDS_H);
    DisplayRender(display);
  }
}
