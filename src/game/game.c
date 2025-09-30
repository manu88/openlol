#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "bytes.h"
#include "console.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "game_envir.h"
#include "geometry.h"
#include "pak_file.h"
#include "render.h"
#include "script.h"
#include "script_builtins.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <_string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

static int GameRun(GameContext *gameCtx);
static int GameInit(GameContext *gameCtx);

static uint16_t callbackGetDirection(EMCInterpreter *interp) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksGetDirection will return %X\n", ctx->level->orientation);
  return ctx->level->orientation;
}

static char dialOrMsgBuffer[1024];
static void callbackPlayDialogue(EMCInterpreter *interp, int16_t charId,
                                 int16_t mode, uint16_t strId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksPlayDialogue charId=%i, mode=%i stringID=%i\n", charId, mode,
         strId);

  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(strId, &useLevelFile);
  printf("real string ID=%i, levelFile?%i\n", realStringId, useLevelFile);

  LangHandleGetString(&ctx->level->levelLang, realStringId, dialOrMsgBuffer,
                      sizeof(dialOrMsgBuffer));
  printf("DIAL: '%s'\n", dialOrMsgBuffer);
}

static void callbackPrintMessage(EMCInterpreter *interp, uint16_t type,
                                 uint16_t strId, uint16_t soundId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksPrintMessage type=%i stringID=%i soundID=%i\n", type, strId,
         soundId);
  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(strId, &useLevelFile);
  printf("real string ID=%i, levelFile?%i\n", realStringId, useLevelFile);
  LangHandleGetString(&ctx->level->levelLang, realStringId, dialOrMsgBuffer,
                      sizeof(dialOrMsgBuffer));
  printf("MSG: '%s'\n", dialOrMsgBuffer);
}

static uint16_t callbackGetGlobalVar(EMCInterpreter *interp, EMCGlobalVarID id,
                                     uint16_t a) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacks GetGlobalVar id=%i\n", id);
  switch (id) {
  case EMCGlobalVarID_CurrentBlock:
    return 1;
  case EMCGlobalVarID_CurrentDir:
    return ctx->level->orientation;
  case EMCGlobalVarID_CurrentLevel:
  case EMCGlobalVarID_ItemInHand:
  case EMCGlobalVarID_Brightness:
  case EMCGlobalVarID_Credits:
  case EMCGlobalVarID_6:
  case EMCGlobalVarID_7_Unused:
    assert(0);
  case EMCGlobalVarID_UpdateFlags:
  case EMCGlobalVarID_OilLampStatus:
  case EMCGlobalVarID_SceneDefaultUpdate:
  case EMCGlobalVarID_CompassBroken:
  case EMCGlobalVarID_DrainMagic:
  case EMCGlobalVarID_SpeechVolume:
  case EMCGlobalVarID_AbortTIMFlag:
    break;
  }
  return 0;
}

static uint16_t callbackSetGlobalVar(EMCInterpreter *interp, EMCGlobalVarID id,
                                     uint16_t a, uint16_t b) {
  printf("callbacks SetGlobalVar id=%i %i %i\n", id, a, b);
  switch (id) {
  case EMCGlobalVarID_CurrentBlock: {
    uint16_t x = 0;
    uint16_t y = 0;
    BlockGetCoordinates(&x, &y, b, 0x80, 0x80);
    uint16_t xx = b & 0x1F;
    uint16_t yx = b >> 5;
    printf("x=%x y=%x\n", xx, yx);
    break;
  }
  case EMCGlobalVarID_CurrentDir:
  case EMCGlobalVarID_CurrentLevel:
  case EMCGlobalVarID_ItemInHand:
  case EMCGlobalVarID_Brightness:
  case EMCGlobalVarID_Credits:
  case EMCGlobalVarID_6:
  case EMCGlobalVarID_7_Unused:
    assert(0);
  case EMCGlobalVarID_UpdateFlags:
  case EMCGlobalVarID_OilLampStatus:
  case EMCGlobalVarID_SceneDefaultUpdate:
  case EMCGlobalVarID_CompassBroken:
  case EMCGlobalVarID_DrainMagic:
  case EMCGlobalVarID_SpeechVolume:
  case EMCGlobalVarID_AbortTIMFlag:
    break;
  }
  return 1;
}

static void callbackLoadLangFile(EMCInterpreter *interp, const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLangFile '%s'\n", file);
  GameFile langFile = {0};
  assert(GameEnvironmentGetLangFile(&langFile, "LEVEL01"));
  if (!langFile.buffer) {
    return;
  }
  if (!LangHandleFromBuffer(&gameCtx->level->levelLang, langFile.buffer,
                            langFile.bufferSize)) {
    printf("LangHandleFromBuffer error\n");
  }
}

static void callbackLoadCMZ(EMCInterpreter *interp, const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadCMZ '%s'\n", file);
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));
  assert(
      MazeHandleFromBuffer(&gameCtx->level->mazHandle, f.buffer, f.bufferSize));
}

static void callbackLoadLevelShapes(EMCInterpreter *interp, const char *shpFile,
                                    const char *datFile) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLevelShapes '%s' '%s'\n", shpFile, datFile);
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, shpFile));
    assert(SHPHandleFromBuffer(&gameCtx->level->shpHandle, f.buffer,
                               f.bufferSize));
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, datFile));
    assert(DatHandleFromBuffer(&gameCtx->level->datHandle, f.buffer,
                               f.bufferSize));
  }
}

static void callbackLoadLevelGraphics(EMCInterpreter *interp,
                                      const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLevelGraphics '%s'\n", file);
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileWithExt(&f, file, "VCN"));
    assert(VCNHandleFromLCWBuffer(&gameCtx->level->vcnHandle, f.buffer,
                                  f.bufferSize));
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileWithExt(&f, file, "VMP"));
    assert(VMPHandleFromLCWBuffer(&gameCtx->level->vmpHandle, f.buffer,
                                  f.bufferSize));
  }
  // files: VCF VCN VMP
}

static void installCallbacks(EMCInterpreter *interp) {
  interp->callbacks.EMCInterpreterCallbacks_GetDirection = callbackGetDirection;
  interp->callbacks.EMCInterpreterCallbacks_PlayDialogue = callbackPlayDialogue;
  interp->callbacks.EMCInterpreterCallbacks_PrintMessage = callbackPrintMessage;
  interp->callbacks.EMCInterpreterCallbacks_GetGlobalVar = callbackGetGlobalVar;
  interp->callbacks.EMCInterpreterCallbacks_SetGlobalVar = callbackSetGlobalVar;
  interp->callbacks.EMCInterpreterCallbacks_LoadLangFile = callbackLoadLangFile;
  interp->callbacks.EMCInterpreterCallbacks_LoadCMZ = callbackLoadCMZ;
  interp->callbacks.EMCInterpreterCallbacks_LoadLevelShapes =
      callbackLoadLevelShapes;
  interp->callbacks.EMCInterpreterCallbacks_LoadLevelGraphics =
      callbackLoadLevelGraphics;
}

int cmdGame(int argc, char *argv[]) {
  assert(GameEnvironmentInit("data"));
  assert(GameEnvironmentLoadChapter(1));
  if (argc < 3) {
    printf("game wall-file ini-file inf-file\n");
    return 0;
  }

  const char *iniFile = argv[0];
  const char *infFile = argv[1];
  const char *wllFile = argv[2];

  GameContext gameCtx = {0};
  if (!GameInit(&gameCtx)) {
    return 1;
  }

  LevelContext levelCtx = {0};
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
    uint8_t *buffer = readBinaryFile(iniFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);
    INFScriptInit(&gameCtx.iniScript);
    if (!INFScriptFromBuffer(&gameCtx.iniScript, buffer, fileSize)) {
      printf("INFScriptFromBuffer error\n");
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
    INFScriptInit(&gameCtx.script);
    if (!INFScriptFromBuffer(&gameCtx.script, buffer, fileSize)) {
      printf("INFScriptFromBuffer error\n");
      return 1;
    }
  }

  gameCtx.level = &levelCtx;
  installCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;
  GameRun(&gameCtx);
  LevelContextRelease(&levelCtx);
  GameContextRelease(&gameCtx);

  printf("GameEnvironmentRelease\n");
  GameEnvironmentRelease();
  return 0;
}

void LevelContextRelease(LevelContext *levelCtx) {
  VMPHandleRelease(&levelCtx->vmpHandle);
  VCNHandleRelease(&levelCtx->vcnHandle);
  MazeHandleRelease(&levelCtx->mazHandle);
  SHPHandleRelease(&levelCtx->shpHandle);
}

static int processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
  LevelContext *ctx = gameCtx->level;
  int shouldUpdate = 1;
  switch (e->key.keysym.sym) {
  case SDLK_ESCAPE:
    gameCtx->consoleHasFocus = 1;
    printf("set console focus\n");
    SDL_StartTextInput();
    break;
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
  setupConsole(gameCtx);
  snprintf(gameCtx->cmdBuffer, 1024, "> ");
  return 1;
}

static void renderStatLine(GameContext *gameCtx, const char *line, int x,
                           int y) {
  if (strlen(line) == 0) {
    return;
  }
  gameCtx->textSurface = TTF_RenderUTF8_Solid(gameCtx->font, line,
                                              (SDL_Color){255, 255, 255, 255});

  gameCtx->textTexture =
      SDL_CreateTextureFromSurface(gameCtx->renderer, gameCtx->textSurface);
  SDL_Rect dstrect = {x, y, gameCtx->textSurface->w, gameCtx->textSurface->h};
  SDL_RenderCopy(gameCtx->renderer, gameCtx->textTexture, NULL, &dstrect);
  SDL_DestroyTexture(gameCtx->textTexture);
  SDL_FreeSurface(gameCtx->textSurface);
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
  snprintf(textStatsBuffer, sizeof(textStatsBuffer),
           "block: %i  newblockpos: %i",
           BlockFromCoords(ctx->partyPos.x, ctx->partyPos.y),
           BlockCalcNewPosition(0, ctx->orientation));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "console mode: %i",
           gameCtx->consoleHasFocus);
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  renderStatLine(gameCtx, gameCtx->cmdBuffer, statsPosX, statsPosY);
}

static int runINIScript(GameContext *gameCtx) {
  EMCData dat = {0};
  EMCDataLoad(&dat, &gameCtx->iniScript);
  EMCState iniState = {0};
  EMCStateInit(&iniState, &dat);
  EMCStateSetOffset(&iniState, 0);
  EMCStateStart(&iniState, 0);
  while (EMCInterpreterIsValid(&gameCtx->interp, &iniState)) {
    if (EMCInterpreterRun(&gameCtx->interp, &iniState) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
  }
  return 1;
}

static int runLevelInitScript(GameContext *gameCtx) {
  return runScript(gameCtx, -1);
}

int runScript(GameContext *gameCtx, int function) {
  EMCData dat = {0};
  EMCDataLoad(&dat, &gameCtx->script);
  EMCState state = {0};
  EMCStateInit(&state, &dat);
  EMCStateSetOffset(&state, 0);
  if (function > 0) {
    if (!EMCStateStart(&state, function)) {
      printf("EMCInterpreterStart: invalid\n");
    }
  }

  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    if (EMCInterpreterRun(&gameCtx->interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
  }
  return 1;
}

static int GameRun(GameContext *gameCtx) {
  LevelContext *ctx = gameCtx->level;
  ctx->partyPos.x = 13;
  ctx->partyPos.y = 18;
  ctx->orientation = North;

  int quit = 0;
  int shouldUpdate = 1;

  printf("START runINIScript\n");
  runINIScript(gameCtx);
  printf("DONE runINIScript\n");
  runLevelInitScript(gameCtx);
  // Event loop
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
    if (gameCtx->consoleHasFocus) {
      shouldUpdate = processConsoleInputs(gameCtx, &e);
    } else {
      if (e.type == SDL_KEYDOWN) {
        shouldUpdate = processGameInputs(gameCtx, &e);
      }
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
