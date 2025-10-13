#include "game_callbacks.h"
#include "formats/format_tim.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint16_t callbackGetDirection(EMCInterpreter *interp) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  return ctx->orientation;
}

static void callbackPlayDialogue(EMCInterpreter *interp, int16_t charId,
                                 int16_t mode, uint16_t strId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksPlayDialogue charId=%i, mode=%i stringID=%i\n", charId, mode,
         strId);

  GameContextGetString(ctx, strId, ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  ctx->dialogText = ctx->dialogTextBuffer;
}

static void callbackPrintMessage(EMCInterpreter *interp, uint16_t type,
                                 uint16_t strId, uint16_t soundId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  printf("callbacksPrintMessage type=%i stringID=%i soundID=%i\n", type, strId,
         soundId);

  GameContextGetString(ctx, strId, ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
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
  char pakFile[12] = "";
  strncpy(pakFile, shpFile, 12);

  pakFile[strlen(pakFile) - 1] = 'K';
  pakFile[strlen(pakFile) - 2] = 'A';
  pakFile[strlen(pakFile) - 3] = 'P';
  printf("using pak file '%s'\n", pakFile);
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, shpFile, pakFile));
    // assert(GameEnvironmentGetFile(&f, shpFile));
    assert(SHPHandleFromBuffer(&gameCtx->level->shpHandle, f.buffer,
                               f.bufferSize));
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, datFile, pakFile));
    // assert(GameEnvironmentGetFile(&f, datFile));
    assert(DatHandleFromBuffer(&gameCtx->level->datHandle, f.buffer,
                               f.bufferSize));
  }
}

static void callbackLoadLevel(EMCInterpreter *interp, uint16_t levelNum,
                              uint16_t startBlock, uint16_t startDir) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLevel %i %X %X\n", levelNum, startBlock, startDir);
  GameContextLoadLevel(gameCtx, levelNum, startBlock, startDir);
}

static void callbackSetGameFlag(EMCInterpreter *interp, uint16_t flag,
                                uint16_t val) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackGameFlag Set %X %X\n", flag, val);
  if (val) {
    GameContextSetGameFlag(gameCtx, flag, val);
  } else {
    GameContextResetGameFlag(gameCtx, flag);
  }
}

static uint16_t callbackTestGameFlag(EMCInterpreter *interp, uint16_t flag) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackGameFlag Test %X\n", flag);
  return GameContextGetGameFlag(gameCtx, flag);
}

static void callbackLoadLevelGraphics(EMCInterpreter *interp,
                                      const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadLevelGraphics '%s'\n", file);
  char pakFile[12] = "";
  snprintf(pakFile, 12, "%s.PAK", file);
  char fileName[12] = "";
  {
    GameFile f = {0};
    snprintf(fileName, 12, "%s.VCN", file);
    assert(GameEnvironmentGetFileFromPak(&f, fileName, pakFile));
    // assert(GameEnvironmentGetFileWithExt(&f, file, "VCN"));
    assert(VCNHandleFromLCWBuffer(&gameCtx->level->vcnHandle, f.buffer,
                                  f.bufferSize));

    gameCtx->timAnimator.defaultPalette = gameCtx->level->vcnHandle.palette;
  }
  {
    GameFile f = {0};
    snprintf(fileName, 12, "%s.VMP", file);
    assert(GameEnvironmentGetFileFromPak(&f, fileName, pakFile));
    // assert(GameEnvironmentGetFileWithExt(&f, file, "VMP"));
    assert(VMPHandleFromLCWBuffer(&gameCtx->level->vmpHandle, f.buffer,
                                  f.bufferSize));
  }
}

static void callbackLoadTimScript(EMCInterpreter *interp, uint16_t scriptId,
                                  const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackLoadTimScript %x :'%s'.TIM\n", scriptId, file);
  GameTimAnimatorLoadTim(&gameCtx->timAnimator, scriptId, file);
}

static void callbackRunTimScript(EMCInterpreter *interp, uint16_t scriptId,
                                 uint16_t loop) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackRunTimScript scriptId=%X loop=%X\n", scriptId, loop);
  if (!gameCtx->timAnimator.tim[scriptId].avtl) {
    printf("NO TIM loaded\n");
    return;
  }
  GameContextSetState(gameCtx, GameState_TimAnimation);
  GameTimAnimatorRunTim(&gameCtx->timAnimator, scriptId);
}

static void callbackReleaseTimScript(EMCInterpreter *interp,
                                     uint16_t scriptId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackReleaseTimScript script=%X\n", scriptId);

  GameTimAnimatorReleaseTim(&gameCtx->timAnimator, scriptId);
}

static uint16_t callbackGetItemInHand(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return gameCtx->itemInHand;
}

static void callbackAllocItemProperties(EMCInterpreter *interp, uint16_t size) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->items = malloc(size * sizeof(Item));
  assert(gameCtx->items);
  gameCtx->itemsCount = size;
}

static char c[128] = "";
static void callbackSetItemProperty(EMCInterpreter *interp, uint16_t index,
                                    uint16_t stringId, uint16_t shapeId,
                                    uint16_t type, uint16_t scriptFun,
                                    uint16_t might, uint16_t skill,
                                    uint16_t protection, uint16_t flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(index < gameCtx->itemsCount);

  gameCtx->items[index].shapeId = shapeId;
  gameCtx->items[index].stringId = stringId;
  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(stringId, &useLevelFile);
  assert(useLevelFile == 0);

  LangHandleGetString(&gameCtx->lang, realStringId, c, sizeof(c));

  printf("callbackSetItemProperty id=%02x shp=%03i typ=%04X flags %04X %s\n",
         index, shapeId, type, flags, c);
}

void GameContextInstallCallbacks(EMCInterpreter *interp) {
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
  interp->callbacks.EMCInterpreterCallbacks_SetGameFlag = callbackSetGameFlag;
  interp->callbacks.EMCInterpreterCallbacks_TestGameFlag = callbackTestGameFlag;
  interp->callbacks.EMCInterpreterCallbacks_LoadTimScript =
      callbackLoadTimScript;
  interp->callbacks.EMCInterpreterCallbacks_RunTimScript = callbackRunTimScript;
  interp->callbacks.EMCInterpreterCallbacks_ReleaseTimScript =
      callbackReleaseTimScript;
  interp->callbacks.EMCInterpreterCallbacks_GetItemInHand =
      callbackGetItemInHand;
  interp->callbacks.EMCInterpreterCallbacks_AllocItemProperties =
      callbackAllocItemProperties;

  interp->callbacks.EMCInterpreterCallbacks_SetItemProperty =
      callbackSetItemProperty;
}
