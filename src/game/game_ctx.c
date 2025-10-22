#include "game_ctx.h"
#include "SDL_render.h"
#include "dbg_server.h"
#include "formats/format_cps.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_envir.h"
#include "game_tim_animator.h"
#include "render.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int runScript(GameContext *gameCtx, INFScript *script);

int GameContextInit(GameContext *gameCtx, Language lang) {
  memset(gameCtx, 0, sizeof(GameContext));
  gameCtx->language = lang;
  gameCtx->state = GameState_PlayGame;
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PARCH.CPS"));

    if (CPSImageFromBuffer(&gameCtx->mapBackground, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get mapBackground Data\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "PLAYFLD.CPS"));

    if (CPSImageFromBuffer(&gameCtx->playField, f.buffer, f.bufferSize) == 0) {
      printf("unable to get playFieldData\n");
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFile(&f, "ITEMICN.SHP"));
    if (SHPHandleFromCompressedBuffer(&gameCtx->itemShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get ITEMICN.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFileWithExt(
        &f, "LANDS", LanguageGetExtension(gameCtx->language)));
    if (LangHandleFromBuffer(&gameCtx->lang, f.buffer, f.bufferSize) == 0) {
      printf("unable to get LANDS.%s\n",
             LanguageGetExtension(gameCtx->language));
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, "AUTOBUT.SHP"));
    if (SHPHandleFromCompressedBuffer(&gameCtx->automapShapes, f.buffer,
                                      f.bufferSize) == 0) {
      printf("unable to get AUTOBUT.SHP\n");
      assert(0);
    }
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetStartupFile(&f, "ITEM.INF"));
    if (INFScriptFromBuffer(&gameCtx->itemScript, f.buffer, f.bufferSize) ==
        0) {
      printf("unable to get ITEMS.INF\n");
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

  gameCtx->backgroundPixBuf = SDL_CreateTexture(
      gameCtx->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (gameCtx->backgroundPixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
    return 1;
  }
  gameCtx->foregroundPixBuf = SDL_CreateTexture(
      gameCtx->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (gameCtx->foregroundPixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
    return 1;
  }

  GameFile f = {0};
  assert(GameEnvironmentGetStartupFile(&f, "FONT6P.FNT"));
  // assert(GameEnvironmentGetGeneralFile(&f, "FONT6P.FNT"));

  if (FNTHandleFromBuffer(&gameCtx->defaultFont, f.buffer, f.bufferSize) == 0) {
    printf("unable to get FONT6P.FNT data\n");
  }

  AnimatorInit(&gameCtx->animator, gameCtx->foregroundPixBuf);
  GameTimInterpreterInit(&gameCtx->timInterpreter, &gameCtx->animator);
  gameCtx->timInterpreter.timInterpreter.callbackCtx = gameCtx;
  gameCtx->dialogTextBuffer = malloc(DIALOG_BUFFER_SIZE);
  assert(gameCtx->dialogTextBuffer);

  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, "GERIM.CPS", "O01A.PAK"));
    CPSImage img = {0};
    assert(CPSImageFromBuffer(&img, f.buffer, f.bufferSize));
    gameCtx->defaultPalette = malloc(img.paletteSize);
    memcpy(gameCtx->defaultPalette, img.palette, img.paletteSize);
    CPSImageRelease(&img);
  }

  gameCtx->itemsInGame = malloc(MAX_IN_GAME_ITEMS * sizeof(GameObject));
  assert(gameCtx->itemsInGame);
  memset(gameCtx->itemsInGame, 0, MAX_IN_GAME_ITEMS * sizeof(GameObject));

  DBGServerInit();
  return 1;
}

void GameContextRelease(GameContext *gameCtx) {
  DBGServerRelease();
  SDL_DestroyRenderer(gameCtx->renderer);
  SDL_DestroyWindow(gameCtx->window);

  SDL_DestroyTexture(gameCtx->backgroundPixBuf);
  SDL_DestroyTexture(gameCtx->foregroundPixBuf);

  PAKFileRelease(&gameCtx->generalPak);
  CPSImageRelease(&gameCtx->playField);
  INFScriptRelease(&gameCtx->script);
  free(gameCtx->dialogTextBuffer);
  SDL_FreeCursor(gameCtx->cursor);

  for (int i = 0; i < 3; i++) {
    if (gameCtx->buttonText[i]) {
      free(gameCtx->buttonText[i]);
    }
  }
  free(gameCtx->defaultPalette);
}

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId) {
  if (itemId == 0) {
    return 0;
  }

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    if (ctx->inventory[i] == 0) {
      ctx->inventory[i] = GameContextCreateItem(ctx, itemId);
      return 1;
    }
  }
  return 0;
}

static int runScript(GameContext *gameCtx, INFScript *script) {
  EMCState state = {0};
  EMCStateInit(&state, script);
  EMCStateSetOffset(&state, 0);
  EMCStateStart(&state, 0);
  while (EMCInterpreterIsValid(&gameCtx->interp, &state)) {
    EMCInterpreterRun(&gameCtx->interp, &state);
  }
  return 1;
}

static int runCompleteScript(GameContext *ctx, const char *name) {
  GameFile f;
  assert(GameEnvironmentGetStartupFile(&f, name));
  INFScript script = {0};
  assert(INFScriptFromBuffer(&script, f.buffer, f.bufferSize));
  return runScript(ctx, &script);
}

int GameContextStartup(GameContext *ctx) {
  return runCompleteScript(ctx, "ONETIME.INF");
}

int GameContextLoadLevel(GameContext *ctx, int levelNum) {
  ctx->dialogText = NULL;

  GameEnvironmentLoadLevel(levelNum);
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

  runScript(ctx, &ctx->iniScript);
  GameContextRunLevelInitScript(ctx);

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
  gameCtx->prevState = gameCtx->state;
  gameCtx->state = newState;
  if (gameCtx->state == GameState_ShowInventory ||
      gameCtx->state == GameState_ShowMap) {
    gameCtx->controlDisabled = 1;
  } else {
    gameCtx->controlDisabled = 0;
  }
}

uint8_t GameContextGetNumChars(const GameContext *ctx) {
  uint8_t c = 0;
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    if (ctx->chars[i].id != 0) {
      c++;
    }
  }
  return c;
}

uint16_t GameContextGetString(const GameContext *ctx, uint16_t stringId,
                              char *outBuffer, size_t outBufferSize) {
  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(stringId, &useLevelFile);
  if (useLevelFile) {
    return LangHandleGetString(&ctx->level->levelLang, realStringId, outBuffer,
                               outBufferSize);
  }
  return LangHandleGetString(&ctx->lang, realStringId, outBuffer,
                             outBufferSize);
}

uint16_t GameContextGetLevelName(const GameContext *gameCtx, char *outBuffer,
                                 size_t outBufferSize) {
  return GameContextGetString(gameCtx,
                              STR_FIST_LEVEL_NAME_INDEX + gameCtx->levelId - 1,
                              outBuffer, outBufferSize);
}

uint16_t GameContextGetItemSHPFrameIndex(GameContext *gameCtx,
                                         uint16_t itemId) {
  return gameCtx->itemProperties[itemId].shapeId;
}

void GameContextInitSceneDialog(GameContext *gameCtx) {
  GameContextSetState(gameCtx, GameState_GrowDialogBox);
  gameCtx->controlDisabled = 1;
}

void GameContextCleanupSceneDialog(GameContext *gameCtx) {
  printf("GameContextCleanupSceneDialog\n");
  if (gameCtx->showBigDialog) {
    GameContextSetState(gameCtx, GameState_ShrinkDialogBox);
  }
  gameCtx->controlDisabled = 0;
  for (int i = 0; i < 3; i++) {
    if (gameCtx->buttonText[i]) {
      free(gameCtx->buttonText[i]);
      gameCtx->buttonText[i] = NULL;
    }
  }
  gameCtx->dialogText = NULL;
}

uint16_t GameContextCreateItem(GameContext *gameCtx, uint16_t itemType) {
  int slot = -1;
  for (int i = 1; i < MAX_IN_GAME_ITEMS; i++) {
    GameObject *item = gameCtx->itemsInGame + i;
    if (item->itemPropertyIndex == 0) {
      slot = i;
      break;
    }
  }
  assert(slot != -1 && slot != MAX_IN_GAME_ITEMS);
  GameObject *item = gameCtx->itemsInGame + slot;
  item->itemPropertyIndex = itemType;
  return slot;
}

void GameContextDeleteItem(GameContext *gameCtx, uint16_t itemIndex) {
  memset(&gameCtx->itemsInGame[itemIndex], 0, sizeof(GameObject));
}

void GameContextUpdateCursor(GameContext *gameCtx) {
  uint16_t itemId =
      gameCtx->itemsInGame[gameCtx->itemIndexInHand].itemPropertyIndex;
  uint16_t frameId =
      itemId ? GameContextGetItemSHPFrameIndex(gameCtx, itemId) : 0;
  createCursorForItem(gameCtx, frameId);

  if (itemId == 0) {
    return;
  }

  GameContextGetString(gameCtx, STR_TAKEN_INDEX, gameCtx->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);
  char *format = strdup(gameCtx->dialogTextBuffer);

  uint16_t stringId = gameCtx->itemProperties[itemId].stringId;
  GameContextGetString(gameCtx, stringId, gameCtx->dialogTextBuffer,
                       DIALOG_BUFFER_SIZE);

  snprintf(gameCtx->dialogTextBuffer, DIALOG_BUFFER_SIZE, format,
           gameCtx->dialogTextBuffer);
  free(format);
  gameCtx->dialogText = gameCtx->dialogTextBuffer;
}
