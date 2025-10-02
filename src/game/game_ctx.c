#include "game_ctx.h"
#include "console.h"
#include <assert.h>

int GameContextInit(GameContext *gameCtx) {
  PAKFileInit(&gameCtx->generalPak);
  if (PAKFileRead(&gameCtx->generalPak, "data/GENERAL.PAK") == 0) {
    printf("unable to read 'data/GENERAL.PAK' file\n");
    return 0;
  }

  int index = PakFileGetEntryIndex(&gameCtx->generalPak, "PLAYFLD.CPS");
  if (index == -1) {
    printf("unable to get 'PLAYFLD.CPS' file from general pak\n");
  }
  uint8_t *playFieldData = PakFileGetEntryData(&gameCtx->generalPak, index);
  if (CPSImageFromFile(&gameCtx->playField, playFieldData,
                       gameCtx->generalPak.entries[index].fileSize) == 0) {
    printf("unable to get playFieldData\n");
  }
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }
  if (TTF_Init() == -1) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }

  gameCtx->font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 18);
  if (!gameCtx->font) {
    printf("unable to create font\n"
           "SDL_Error: %s\n",
           TTF_GetError());
    return 0;
  }

  gameCtx->window = SDL_CreateWindow("Lands Of Lore", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                     SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!gameCtx->window) {
    printf("Window could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  gameCtx->renderer =
      SDL_CreateRenderer(gameCtx->window, -1, SDL_RENDERER_ACCELERATED);
  if (!gameCtx->renderer) {
    printf("Renderer could not be created!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }

  gameCtx->dialogTextBuffer = malloc(DIALOG_BUFFER_SIZE);
  assert(gameCtx->dialogTextBuffer);
  setupConsole(gameCtx);
  snprintf(gameCtx->cmdBuffer, 1024, "> ");
  return 1;
}

void GameContextRelease(GameContext *gameCtx) {
  SDL_DestroyRenderer(gameCtx->renderer);
  SDL_DestroyWindow(gameCtx->window);
  if (gameCtx->font) {
    TTF_CloseFont(gameCtx->font);
  }
  PAKFileRelease(&gameCtx->generalPak);
  CPSImageRelease(&gameCtx->playField);
  INFScriptRelease(&gameCtx->script);
  free(gameCtx->dialogTextBuffer);
}
