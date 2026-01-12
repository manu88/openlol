#include "display.h"
#include "game_ctx.h"
#include "game_envir.h"
#include <string.h>

static int initSDL(Display *renderer) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }

  renderer->window =
      SDL_CreateWindow("Lands Of Lore", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, PIX_BUF_WIDTH * SCREEN_FACTOR,
                       PIX_BUF_HEIGHT * SCREEN_FACTOR, SDL_WINDOW_SHOWN);
  if (!renderer->window) {
    printf("Window could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }
  renderer->renderer =
      SDL_CreateRenderer(renderer->window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer->renderer) {
    printf("Renderer could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }

  renderer->pixBuf = SDL_CreateTexture(
      renderer->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (renderer->pixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
    return 0;
  }
  return 1;
}

int RenderingContextInit(Display *renderer) {
  memset(renderer, 0, sizeof(Display));

  if (!initSDL(renderer)) {
    return 0;
  }
  renderer->shouldUpdate = 1;

  renderer->dialogTextBuffer = malloc(DIALOG_BUFFER_SIZE);
  assert(renderer->dialogTextBuffer);

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "TITLE.CPS"));

    if (CPSImageFromBuffer(&renderer->gameTitle, f.buffer, f.bufferSize) == 0) {
      printf("unable to get Title.CPS Data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PARCH.CPS"));

    if (CPSImageFromBuffer(&renderer->mapBackground, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get mapBackground Data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PLAYFLD.CPS"));

    if (CPSImageFromBuffer(&renderer->playField, f.buffer, f.bufferSize) == 0) {
      printf("unable to get playFieldData\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFile(&f, "ITEMICN.SHP"));
    if (SHPHandleFromCompressedBuffer(&renderer->itemShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get ITEMICN.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "GAMESHP.SHP"));
    if (SHPHandleFromCompressedBuffer(&renderer->gameShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get GAMESHP.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "AUTOBUT.SHP"));
    if (SHPHandleFromCompressedBuffer(&renderer->automapShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get AUTOBUT.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT9PN.FNT"));
    if (FNTHandleFromBuffer(&renderer->defaultFont, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get FONT9P.FNT data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "FONT6P.FNT"));

    if (FNTHandleFromBuffer(&renderer->font6p, f.buffer, f.bufferSize) == 0) {
      printf("unable to get FONT6P.FNT data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, "GERIM.CPS", "O01A.PAK"));
    CPSImage img = {0};
    assert(CPSImageFromBuffer(&img, f.buffer, f.bufferSize));
    renderer->defaultPalette = malloc(img.paletteSize);
    memcpy(renderer->defaultPalette, img.palette, img.paletteSize);
    CPSImageRelease(&img);
  }

  return 1;
}

void RenderingContextRelease(Display *renderer) {
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
