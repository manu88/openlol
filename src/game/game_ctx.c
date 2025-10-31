#include "game_ctx.h"
#include "SDL_render.h"
#include "bytes.h"
#include "dbg_server.h"
#include "formats/format_cps.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_envir.h"
#include "game_tim_animator.h"
#include "menu.h"
#include "render.h"
#include "script.h"
#include <_string.h>
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int runScript(GameContext *gameCtx, INFScript *script);

int GameContextSetSavDir(GameContext *gameCtx, const char *path) {
  if (path == NULL) {
    gameCtx->savDir = "./";
  } else {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
      assert(0);
      return 0;
    }
    if (S_ISDIR(path_stat.st_mode)) {
      gameCtx->savDir = (char *)path;
    } else {
      gameCtx->savDir = dirname((char *)path);
    }
  }
  return 1;
}

void SAVFilesRelease(SAVFile *files, size_t numFiles) {
  for (int i = 0; i < numFiles; i++) {
    free(files[i].fullpath);
    free(files[i].savName);
  }
  free(files);
}

SAVFile *GameContextListSavFiles(GameContext *gameCtx, size_t *numSavFiles) {
  DIR *dp;
  struct dirent *ep;
  dp = opendir(gameCtx->savDir);
  if (dp == NULL) {
    perror("Couldn't open the directory");
    assert(0);
  }

  const size_t fullPathSize = strlen(gameCtx->savDir) + 32;
  char *fullPath = malloc(fullPathSize);

  SAVFile *files = NULL;
  size_t num = 0;
  while ((ep = readdir(dp)) != NULL) {
    const char *dot = strrchr(ep->d_name, '.');
    if (!dot) {
      continue;
    }
    const char *ext = dot + 1;
    if (strcmp(ext, "DAT") != 0) {
      continue;
    }
    snprintf(fullPath, fullPathSize, "%s/%s", gameCtx->savDir, ep->d_name);
    size_t fileSize;
    size_t bufferSize;
    uint8_t *buffer = readBinaryFile(fullPath, &fileSize, &bufferSize);
    SAVHandle sav = {0};
    SAVHandleFromBuffer(&sav, buffer, bufferSize);

    num++;
    files = realloc(files, num * sizeof(SAVFile));
    assert(files);
    files[num - 1].fullpath = strdup(fullPath);
    files[num - 1].savName = strdup(sav.slot.header->name);

    free(buffer);
  }
  *numSavFiles = num;
  free(fullPath);
  closedir(dp);
  return files;
}

int GameContextInit(GameContext *gameCtx, Language lang) {
  memset(gameCtx, 0, sizeof(GameContext));
  gameCtx->language = lang;
  GameContextSetState(gameCtx, GameState_MainMenu);
  gameCtx->shouldUpdate = 1;
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, "TITLE.CPS"));

    if (CPSImageFromBuffer(&gameCtx->gameTitle, f.buffer, f.bufferSize) == 0) {
      printf("unable to get Title.CPS Data\n");
    }
  }
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

  gameCtx->pixBuf = SDL_CreateTexture(
      gameCtx->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
      PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  if (gameCtx->pixBuf == NULL) {
    printf("Error: %s\n", SDL_GetError());
    return 1;
  }

  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, "FONT9P.FNT"));

  if (FNTHandleFromBuffer(&gameCtx->defaultFont, f.buffer, f.bufferSize) == 0) {
    printf("unable to get FONT6P.FNT data\n");
  }

  AnimatorInit(&gameCtx->animator, gameCtx->pixBuf);
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

int GameContextNewGame(GameContext *gameCtx) {
  gameCtx->levelId = 1;
  gameCtx->credits = 41;
  gameCtx->chars[0].id = -9; // Ak'shel for the win
  snprintf(gameCtx->chars[0].name, 11, "Ak'shel");
  // temp until we get the value from script/tim
  gameCtx->currentBock = 0X24D;
  gameCtx->orientation = North;
  gameCtx->inventory[0] = GameContextCreateItem(gameCtx, 216); // salve
  gameCtx->inventory[1] = GameContextCreateItem(gameCtx, 217); // aloe
  gameCtx->inventory[2] = GameContextCreateItem(gameCtx, 218); // Ginseng
  GameContextLoadChars(gameCtx);
  if (GameContextLoadLevel(gameCtx, gameCtx->levelId)) {

    GameContextSetState(gameCtx, GameState_PlayGame);
    return 1;
  }
  return 0;
}

void GameContextRelease(GameContext *gameCtx) {
  DBGServerRelease();
  SDL_DestroyRenderer(gameCtx->renderer);
  SDL_DestroyWindow(gameCtx->window);

  SDL_DestroyTexture(gameCtx->pixBuf);

  CPSImageRelease(&gameCtx->playField);
  CPSImageRelease(&gameCtx->gameTitle);
  CPSImageRelease(&gameCtx->mapBackground);
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

int GameContextLoadChars(GameContext *gameCtx) {
  char faceFile[11] = "";
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    uint8_t charId =
        gameCtx->chars[i].id > 0 ? gameCtx->chars[i].id : -gameCtx->chars[i].id;
    if (charId == 0) {
      continue;
    }
    snprintf(faceFile, 11, "FACE%02i.SHP", charId);
    GameFile f = {0};
    assert(GameEnvironmentGetGeneralFile(&f, faceFile));
    SHPHandleFromCompressedBuffer(&gameCtx->charFaces[i], f.buffer,
                                  f.bufferSize);
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

void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag) {
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
  if (gameCtx->state == GameState_GameMenu) {
    gameCtx->currentMenu = gameMenu;
  } else if (gameCtx->state == GameState_MainMenu) {
    gameCtx->currentMenu = mainMenu;
  } else if (gameCtx->currentMenu) {
    MenuReset(gameCtx->currentMenu);
    gameCtx->currentMenu = NULL;
  }
  if (gameCtx->state == GameState_ShowInventory ||
      gameCtx->state == GameState_ShowMap ||
      gameCtx->state == GameState_GameMenu) {
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

void GameContextExitGame(GameContext *gameCtx) { gameCtx->_shouldRun = 0; }

int GameContextLoadSaveFile(GameContext *gameCtx, const char *filepath) {
  SAVHandle savHandle = {0};
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    return 0;
  }
  if (!SAVHandleFromBuffer(&savHandle, buffer, readSize)) {
    return 0;
  }

  gameCtx->levelId = savHandle.slot.general->currentLevel;
  memcpy(gameCtx->itemsInGame, savHandle.slot.gameObjects,
         sizeof(GameObject) * MAX_IN_GAME_ITEMS);

  for (int i = 0; i < MAX_IN_GAME_ITEMS; i++) {
    const GameObject *obj = gameCtx->itemsInGame + i;
    if (obj->itemPropertyIndex == 0) {
      continue;
    }
  }
  memcpy(gameCtx->inventory, savHandle.slot.general->inventory,
         INVENTORY_SIZE * sizeof(uint16_t));

  for (int i = 0; i < NUM_CHARACTERS; i++) {
    if (!savHandle.slot.characters[i]->flags) {
      continue;
    }
    memcpy(&gameCtx->chars[i], savHandle.slot.characters[i],
           sizeof(SAVCharacter));
  }
  gameCtx->currentBock = savHandle.slot.general->currentBlock;
  gameCtx->orientation = savHandle.slot.general->currentDirection;
  gameCtx->credits = savHandle.slot.general2->credits;
  gameCtx->itemIndexInHand = savHandle.slot.general->itemIndexInHand;

  SAVHandleGetGameFlags(&savHandle, gameCtx->gameFlags, NUM_GAME_FLAGS);
  GameContextLoadLevel(gameCtx, gameCtx->levelId);
  GameContextUpdateCursor(gameCtx);
  GameContextLoadChars(gameCtx);
  return 1;
}
