#include "game_ctx.h"
#include "console.h"
#include "game_envir.h"
#include <assert.h>

static int runINIScript(GameContext *gameCtx);

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

  {
    int index = PakFileGetEntryIndex(&gameCtx->generalPak, "FONT6P.FNT");
    if (index == -1) {
      printf("unable to get 'FONT6P.FNT' file from general pak\n");
      assert(0);
    }
    uint8_t *font6PData = PakFileGetEntryData(&gameCtx->generalPak, index);
    FNTHandleFromBuffer(&gameCtx->defaultFont, font6PData,
                        gameCtx->generalPak.entries[index].fileSize);
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

int GameContextLoadLevel(GameContext *ctx, int levelNum, uint16_t startBlock,
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
  GameContextRunLevelInitScript(ctx);

  return 1;
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

int GameContextRunLevelInitScript(GameContext *gameCtx) {
  return GameContextRunScript(gameCtx, -1);
}

int GameContextRunScript(GameContext *gameCtx, int function) {
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
