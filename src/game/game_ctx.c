#include "game_ctx.h"
#include "SDL_render.h"
#include "dbg_server.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_envir.h"
#include "game_tim_animator.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static int runINIScript(GameContext *gameCtx);

int GameContextInit(GameContext *gameCtx) {
  memset(gameCtx, 0, sizeof(GameContext));
  PAKFileInit(&gameCtx->generalPak);
  if (PAKFileRead(&gameCtx->generalPak, "data/GENERAL.PAK") == 0) {
    printf("unable to read 'data/GENERAL.PAK' file\n");
    return 0;
  }

  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PLAYFLD.CPS"));

    if (CPSImageFromFile(&gameCtx->playField, f.buffer, f.bufferSize) == 0) {
      printf("unable to get playFieldData\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "ITEMICN.SHP"));
    if (SHPHandleFromCompressedBuffer(&gameCtx->itemShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get ITEMICN.SHP\n");
      assert(0);
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not be initialized!\n"
           "SDL_Error: %s\n",
           SDL_GetError());
    return 0;
  }

  gameCtx->window =
      SDL_CreateWindow("Lands Of Lore", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, PIX_BUF_WIDTH * SCREEN_FACTOR,
                       PIX_BUF_HEIGHT * SCREEN_FACTOR, SDL_WINDOW_SHOWN);
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

  gameCtx->pixBuf = SDL_CreateTexture(
      gameCtx->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (gameCtx->pixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
  }

  GameFile f = {0};
  assert(GameEnvironmentGetGeneralFile(&f, "FONT6P.FNT"));

  if (FNTHandleFromBuffer(&gameCtx->defaultFont, f.buffer, f.bufferSize) == 0) {
    printf("unable to get FONT6P.FNT data\n");
  }

  GameTimAnimatorInit(gameCtx, gameCtx->pixBuf);
  gameCtx->dialogTextBuffer = malloc(DIALOG_BUFFER_SIZE);
  assert(gameCtx->dialogTextBuffer);

  DBGServerInit();
  return 1;
}

void GameContextRelease(GameContext *gameCtx) {
  DBGServerRelease();
  SDL_DestroyRenderer(gameCtx->renderer);
  SDL_DestroyWindow(gameCtx->window);

  SDL_DestroyTexture(gameCtx->pixBuf);

  PAKFileRelease(&gameCtx->generalPak);
  CPSImageRelease(&gameCtx->playField);
  INFScriptRelease(&gameCtx->script);
  free(gameCtx->dialogTextBuffer);
}

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId) {
  if (itemId == 0 || itemId > MAX_ITEM_ID) {
    return 0;
  }

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    if (ctx->inventory[i] == 0) {
      ctx->inventory[i] = itemId;
      return 1;
    }
  }
  return 0;
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
    EMCInterpreterRun(&gameCtx->interp, &iniState);
  }
  return 1;
}

int GameContextRunLevelInitScript(GameContext *gameCtx) {
  return GameContextRunScript(gameCtx, -1);
}

int GameContextRunScript(GameContext *gameCtx, int function) {
  EMCStateInit(&gameCtx->interpState, &gameCtx->script);
  EMCStateSetOffset(&gameCtx->interpState, 0);
  if (function > 0) {
    if (!EMCStateStart(&gameCtx->interpState, function)) {
      printf("EMCInterpreterStart: invalid\n");
      return 0;
    }
  }
  return 1;
}

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  return ((gameCtx->gameFlags[flag >> 3] >> (flag & 7)) & 1);
}

void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag, uint16_t val) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  gameCtx->gameFlags[flag >> 3] |= (1 << (flag & 7));
}

void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag) {
  assert((flag >> 3) >= 0 && (flag >> 3) < sizeof(gameCtx->gameFlags));
  gameCtx->gameFlags[flag >> 3] &= ~(1 << (flag & 7));
}

void GameContextSetState(GameContext *gameCtx, GameState newState) {
  printf("GameContextSetState from %i to %i\n", gameCtx->state, newState);
  gameCtx->state = newState;
}

uint8_t GameContextGetNumChars(const GameContext *ctx) {
  uint8_t c = 0;
  for (int i = 0; i < 4; i++) {
    if (ctx->chars[i].id != 0) {
      c++;
    }
  }
  return c;
}
