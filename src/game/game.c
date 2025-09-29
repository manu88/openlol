#include "game.h"
#include "SDL_events.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "bytes.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_inf.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "geometry.h"
#include "pak_file.h"
#include "render.h"
#include "script.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale/_stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

static int GameRun(GameContext *gameCtx);
static int GameInit(GameContext *gameCtx);

int cmdGame(int argc, char *argv[]) {
  if (argc < 7) {
    printf("game maz-file vcn-file vmp-file wall-file dat-file shp-file "
           "inf-file\n");
    return 0;
  }
  const char *mazFile = argv[0];
  const char *vcnFile = argv[1];
  const char *vmpFile = argv[2];
  const char *wllFile = argv[3];

  const char *datFile = argv[4];
  const char *shpFile = argv[5];
  const char *infFile = argv[6];

  GameContext gameCtx = {0};
  if (!GameInit(&gameCtx)) {
    return 1;
  }

  LevelContext levelCtx = {0};
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vcnFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VCNHandleFromLCWBuffer(&levelCtx.vcnHandle, buffer, fileSize)) {
      printf("VCNDataFromLCWBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vmpFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VMPHandleFromLCWBuffer(&levelCtx.vmpHandle, buffer, fileSize)) {
      printf("VMPDataFromLCWBuffer error\n");
      free(buffer);
      return 1;
    }
    free(buffer);
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(mazFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!MazeHandleFromBuffer(&levelCtx.mazHandle, buffer, fileSize)) {
      printf("MazeHandleFromBuffer error\n");
      free(buffer);
      return 1;
    }
    free(buffer);
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(wllFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!WllHandleFromBuffer(&levelCtx.wllHandle, buffer, fileSize)) {
      printf("WllHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(datFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!DatHandleFromBuffer(&levelCtx.datHandle, buffer, fileSize)) {
      printf("DatHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(shpFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!SHPHandleFromBuffer(&levelCtx.shpHandle, buffer, fileSize)) {
      printf("SHPHandleFromBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(infFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);
    if (!INFScriptFromBuffer(&gameCtx.script, buffer, fileSize)) {
      printf("INFScriptFromBuffer error\n");
      return 1;
    }
  }
  printf("Got all files\n");

  gameCtx.level = &levelCtx;
  GameRun(&gameCtx);
  LevelContextRelease(&levelCtx);
  GameContextRelease(&gameCtx);
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static int processGameInputs(LevelContext *ctx, const SDL_Event *e) {
  int shouldUpdate = 1;
  switch (e->key.keysym.sym) {
  case SDLK_z:
    // go front
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.y -= 1;
      break;
    case East:
      ctx->partyPos.x += 1;
      break;
    case South:
      ctx->partyPos.y += 1;
      break;
    case West:
      ctx->partyPos.x -= 1;
      break;
    }
    break;
  case SDLK_s:
    // go back
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.y += 1;
      break;
    case East:
      ctx->partyPos.x -= 1;
      break;
    case South:
      ctx->partyPos.y -= 1;
      break;
    case West:
      ctx->partyPos.x += 1;
      break;
    }
    break;
  case SDLK_q:
    // go left
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.x -= 1;
      break;
    case East:
      ctx->partyPos.y -= 1;
      break;
    case South:
      ctx->partyPos.x += 1;
      break;
    case West:
      ctx->partyPos.y += 1;
      break;
    }
    break;
  case SDLK_d:
    // go right
    shouldUpdate = 1;
    switch (ctx->orientation) {
    case North:
      ctx->partyPos.x += 1;
      break;
    case East:
      ctx->partyPos.y += 1;
      break;
    case South:
      ctx->partyPos.x -= 1;
      break;
    case West:
      ctx->partyPos.y -= 1;
      break;
    }
    break;
  case SDLK_a:
    // turn anti-clockwise
    shouldUpdate = 1;
    ctx->orientation -= 1;
    if ((int)ctx->orientation < 0) {
      ctx->orientation = West;
    }
    break;
  case SDLK_e:
    // turn clockwise
    shouldUpdate = 1;
    ctx->orientation += 1;
    if (ctx->orientation > West) {
      ctx->orientation = North;
    }
    break;
  default:
    break;
  }
  return shouldUpdate;
}

static int GameInit(GameContext *gameCtx) {
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

  return 1;
}

static void renderStatLine(GameContext *gameCtx, const char *line, int x,
                           int y) {
  gameCtx->textSurface = TTF_RenderUTF8_Solid(gameCtx->font, line,
                                              (SDL_Color){255, 255, 255, 255});

  gameCtx->textTexture =
      SDL_CreateTextureFromSurface(gameCtx->renderer, gameCtx->textSurface);
  SDL_Rect dstrect = {x, y, gameCtx->textSurface->w, gameCtx->textSurface->h};
  SDL_RenderCopy(gameCtx->renderer, gameCtx->textTexture, NULL, &dstrect);
  SDL_DestroyTexture(gameCtx->textTexture);
  SDL_FreeSurface(gameCtx->textSurface);
}

uint16_t calcNewBlockPosition(uint16_t curBlock, uint16_t direction) {
  static const int16_t blockPosTable[] = {-32, 1, 32, -1};
  assert(direction < sizeof(blockPosTable) / sizeof(int16_t));
  return (curBlock + blockPosTable[direction]) & 0x3FF;
}

static char textStatsBuffer[512];

static void renderTextStats(GameContext *gameCtx, LevelContext *ctx) {
  int statsPosX = 20;
  int statsPosY = 400;

  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "pose x=%i y=%i o=%i %c",
           ctx->partyPos.x, ctx->partyPos.y, ctx->orientation,
           ctx->orientation == North ? 'N'
                                     : (ctx->orientation == South  ? 'S'
                                        : ctx->orientation == East ? 'E'
                                                                   : 'W'));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "newblockpos: %i",
           calcNewBlockPosition(0, ctx->orientation));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);
}

static int runScript(GameContext *gameCtx, int function) {
  EMCData dat = {0};
  EMCInterpreterLoad(&gameCtx->interp, &gameCtx->script, &dat);
  EMCState state = {0};
  EMCStateInit(&state, &dat);
  EMCStateSetOffset(&state, 400);

  if (!EMCStateStart(&state, function)) {
    printf("EMCInterpreterStart: invalid\n");
  }

  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    if (EMCInterpreterRun(&gameCtx->interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
  }
}

static int GameRun(GameContext *gameCtx) {
  LevelContext *ctx = gameCtx->level;
  ctx->partyPos.x = 13;
  ctx->partyPos.y = 18;
  ctx->orientation = North;

  int quit = 0;
  int shouldUpdate = 1;

  runScript(gameCtx, 400);
  // Event loop
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    } else if (e.type == SDL_KEYDOWN) {
      shouldUpdate = processGameInputs(ctx, &e);
    }
    if (shouldUpdate) {
      memset(ctx->viewConeEntries, 0,
             sizeof(ViewConeEntry) * VIEW_CONE_NUM_CELLS);
      SDL_SetRenderDrawColor(gameCtx->renderer, 0, 0, 0, 0);
      SDL_RenderClear(gameCtx->renderer);
      GameRenderFrame(gameCtx);
      renderTextStats(gameCtx, ctx);

      SDL_RenderPresent(gameCtx->renderer);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
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
}
