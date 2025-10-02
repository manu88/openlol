#include "game.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
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

static int GameRun(GameContext *gameCtx);
static int loadLevel(GameContext *ctx, int level, uint16_t startBlock,
                     uint16_t startDir);
static int runLevelInitScript(GameContext *gameCtx);
static int runINIScript(GameContext *gameCtx);

static uint16_t callbackGetDirection(EMCInterpreter *interp) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksGetDirection will return %X\n", ctx->orientation);
  return ctx->orientation;
}

static void callbackPlayDialogue(EMCInterpreter *interp, int16_t charId,
                                 int16_t mode, uint16_t strId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksPlayDialogue charId=%i, mode=%i stringID=%i\n", charId, mode,
         strId);

  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(strId, &useLevelFile);
  printf("real string ID=%i, levelFile?%i\n", realStringId, useLevelFile);

  LangHandleGetString(&ctx->level->levelLang, realStringId,
                      ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  ctx->dialogText = ctx->dialogTextBuffer;
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
  LangHandleGetString(&ctx->level->levelLang, realStringId,
                      ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  ctx->dialogText = ctx->dialogTextBuffer;
}

static uint16_t callbackGetGlobalVar(EMCInterpreter *interp, EMCGlobalVarID id,
                                     uint16_t a) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacks GetGlobalVar id=%i\n", id);
  switch (id) {
  case EMCGlobalVarID_CurrentBlock:
    return ctx->currentBock;
  case EMCGlobalVarID_CurrentDir:
    return ctx->orientation;
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
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  printf("callbacks SetGlobalVar id=%i %i %i\n", id, a, b);

  switch (id) {
  case EMCGlobalVarID_CurrentBlock: {
    ctx->currentBock = b;
    printf("set current block to %X\n", ctx->currentBock);
    GetGameCoordsFromBlock(ctx->currentBock, &ctx->partyPos.x,
                           &ctx->partyPos.y);
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
    assert(0);
  }
  return 1;
}

static void callbackLoadLangFile(EMCInterpreter *interp, const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLangFile '%s'\n", file);
  GameFile langFile = {0};
  assert(GameEnvironmentGetLangFile(&langFile, file));
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

static void callbackLoadLevel(EMCInterpreter *interp, uint16_t levelNum,
                              uint16_t startBlock, uint16_t startDir) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLevel %i %X %X\n", levelNum, startBlock, startDir);
  loadLevel(gameCtx, levelNum, startBlock, startDir);
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
  interp->callbacks.EMCInterpreterCallbacks_LoadLevel = callbackLoadLevel;
}

static int loadLevel(GameContext *ctx, int levelNum, uint16_t startBlock,
                     uint16_t startDir) {

  GetGameCoordsFromBlock(startBlock, &ctx->partyPos.x, &ctx->partyPos.y);
  ctx->orientation = startDir;
  ctx->dialogText = NULL;

  {
    GameFile f = {0};
    char wllFile[12];
    snprintf(wllFile, 12, "LEVEL%i.WLL", levelNum);
    assert(GameEnvironmentGetFile(&f, wllFile));
    assert(WllHandleFromBuffer(&ctx->level->wllHandle, f.buffer, f.bufferSize));
  }
  {
    GameFile f = {0};
    char iniFile[12];
    snprintf(iniFile, 12, "LEVEL%i.INI", levelNum);
    assert(GameEnvironmentGetFile(&f, iniFile));
    assert(INFScriptFromBuffer(&ctx->iniScript, f.buffer, f.bufferSize));
  }
  {
    GameFile f = {0};
    char infFile[12];
    snprintf(infFile, 12, "LEVEL%i.INF", levelNum);
    assert(GameEnvironmentGetFile(&f, infFile));
    assert(INFScriptFromBuffer(&ctx->script, f.buffer, f.bufferSize));
  }

  printf("START runINIScript\n");
  runINIScript(ctx);
  printf("DONE runINIScript\n");
  runLevelInitScript(ctx);

  return 1;
}

int cmdGame(int argc, char *argv[]) {
  assert(GameEnvironmentInit("data"));

  if (argc < 2) {
    printf("game chapterID levelID\n");
    return 0;
  }

  int chapterId = atoi(argv[0]);
  int levelId = atoi(argv[1]);
  if (chapterId < 1) {
    return 1;
  }
  if (levelId < 1) {
    return 1;
  }
  printf("Loading chapter %i level %i\n", chapterId, levelId);

  assert(GameEnvironmentLoadChapter(chapterId));

  GameContext gameCtx = {0};
  if (!GameContextInit(&gameCtx)) {
    return 1;
  }

  installCallbacks(&gameCtx.interp);
  gameCtx.interp.callbackCtx = &gameCtx;

  LevelContext levelCtx = {0};
  gameCtx.level = &levelCtx;
  loadLevel(&gameCtx, levelId, 0X24D, North);

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

static void clickOnFrontWall(GameContext *gameCtx) {
  uint16_t nextBlock =
      BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation);

  runScript(gameCtx, nextBlock);
}

static int processGameInputs(GameContext *gameCtx, const SDL_Event *e) {
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
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.y -= 1;
      break;
    case East:
      gameCtx->partyPos.x += 1;
      break;
    case South:
      gameCtx->partyPos.y += 1;
      break;
    case West:
      gameCtx->partyPos.x -= 1;
      break;
    }
    break;
  case SDLK_s:
    // go back
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.y += 1;
      break;
    case East:
      gameCtx->partyPos.x -= 1;
      break;
    case South:
      gameCtx->partyPos.y -= 1;
      break;
    case West:
      gameCtx->partyPos.x += 1;
      break;
    }
    break;
  case SDLK_q:
    // go left
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.x -= 1;
      break;
    case East:
      gameCtx->partyPos.y -= 1;
      break;
    case South:
      gameCtx->partyPos.x += 1;
      break;
    case West:
      gameCtx->partyPos.y += 1;
      break;
    }
    break;
  case SDLK_d:
    // go right
    shouldUpdate = 1;
    switch (gameCtx->orientation) {
    case North:
      gameCtx->partyPos.x += 1;
      break;
    case East:
      gameCtx->partyPos.y += 1;
      break;
    case South:
      gameCtx->partyPos.x -= 1;
      break;
    case West:
      gameCtx->partyPos.y -= 1;
      break;
    }
    break;
  case SDLK_a:
    // turn anti-clockwise
    shouldUpdate = 1;
    gameCtx->orientation -= 1;
    if ((int)gameCtx->orientation < 0) {
      gameCtx->orientation = West;
    }
    break;
  case SDLK_e:
    // turn clockwise
    shouldUpdate = 1;
    gameCtx->orientation += 1;
    if (gameCtx->orientation > West) {
      gameCtx->orientation = North;
    }
    break;
  case SDLK_SPACE:
    clickOnFrontWall(gameCtx);
    break;
  default:
    break;
  }
  return shouldUpdate;
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

static void renderDialog(GameContext *gameCtx) {
  if (gameCtx->dialogText) {
    renderStatLine(gameCtx, gameCtx->dialogText, 30, 250);
  }
}
static char textStatsBuffer[512];

static void renderTextStats(GameContext *gameCtx) {
  int statsPosX = 20;
  int statsPosY = 400;

  uint16_t gameX = 0;
  uint16_t gameY = 0;
  GetGameCoords(gameCtx->partyPos.x, gameCtx->partyPos.y, &gameX, &gameY);
  snprintf(textStatsBuffer, sizeof(textStatsBuffer),
           "pose x=%i (%X) y=%i (%X) o=%i %c", gameCtx->partyPos.x, gameX,
           gameCtx->partyPos.y, gameY, gameCtx->orientation,
           gameCtx->orientation == North
               ? 'N'
               : (gameCtx->orientation == South  ? 'S'
                  : gameCtx->orientation == East ? 'E'
                                                 : 'W'));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  gameCtx->currentBock = BlockFromCoords(gameX, gameY);
  snprintf(textStatsBuffer, sizeof(textStatsBuffer),
           "block: %X  newblockpos: %X", gameCtx->currentBock,
           BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "console mode: %i",
           gameCtx->consoleHasFocus);
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  renderStatLine(gameCtx, gameCtx->cmdBuffer, statsPosX, statsPosY);
}

static int runINIScript(GameContext *gameCtx) {
  EMCState iniState = {0};
  EMCStateInit(&iniState, &gameCtx->iniScript);
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
  EMCState state = {0};
  EMCStateInit(&state, &gameCtx->script);
  EMCStateSetOffset(&state, 0);
  if (function > 0) {
    if (!EMCStateStart(&state, function)) {
      printf("EMCInterpreterStart: invalid\n");
      return 0;
    }
  }

  state.regs[5] = gameCtx->currentBock;
  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    if (EMCInterpreterRun(&gameCtx->interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
  }
  return 1;
}

static int GameRun(GameContext *gameCtx) {
  LevelContext *ctx = gameCtx->level;

  int quit = 0;
  int shouldUpdate = 1;

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
      renderTextStats(gameCtx);
      renderDialog(gameCtx);
      SDL_RenderPresent(gameCtx->renderer);
      shouldUpdate = 0;
    }
  }

  SDL_Quit();
  return 0;
}
