#include "game_callbacks.h"
#include "formats/format_cps.h"
#include "formats/format_shp.h"
#include "formats/format_tim.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
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
  GameContextGetString(ctx, strId, ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  ctx->dialogText = ctx->dialogTextBuffer;
}

static void callbackPrintMessage(EMCInterpreter *interp, uint16_t type,
                                 uint16_t strId, uint16_t soundId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  GameContextGetString(ctx, strId, ctx->dialogTextBuffer, DIALOG_BUFFER_SIZE);
  ctx->dialogText = ctx->dialogTextBuffer;
}

static uint16_t callbackGetGlobalVar(EMCInterpreter *interp, EMCGlobalVarID id,
                                     uint16_t a) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
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

  switch (id) {
  case EMCGlobalVarID_CurrentBlock: {
    ctx->currentBock = b;
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
  GameFile langFile = {0};
  assert(GameEnvironmentGetLangFile(&langFile, file));
  if (!langFile.buffer) {
    return;
  }
  if (!LangHandleFromBuffer(&gameCtx->level->levelLang, langFile.buffer,
                            langFile.bufferSize)) {
    printf("LangHandleFromBuffer error\n");
    assert(0);
  }
}

static void callbackLoadCMZ(EMCInterpreter *interp, const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));
  assert(
      MazeHandleFromBuffer(&gameCtx->level->mazHandle, f.buffer, f.bufferSize));
}

static void callbackLoadLevelShapes(EMCInterpreter *interp, const char *shpFile,
                                    const char *datFile) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  char pakFile[12] = "";
  strncpy(pakFile, shpFile, 12);

  pakFile[strlen(pakFile) - 1] = 'K';
  pakFile[strlen(pakFile) - 2] = 'A';
  pakFile[strlen(pakFile) - 3] = 'P';
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, shpFile, pakFile));
    assert(SHPHandleFromBuffer(&gameCtx->level->shpHandle, f.buffer,
                               f.bufferSize));
  }
  {
    GameFile f = {0};
    assert(GameEnvironmentGetFileFromPak(&f, datFile, pakFile));
    assert(DatHandleFromBuffer(&gameCtx->level->datHandle, f.buffer,
                               f.bufferSize));
  }
}

static void callbackLoadLevel(EMCInterpreter *interp, uint16_t levelNum,
                              uint16_t startBlock, uint16_t startDir) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->currentBock = startBlock;
  gameCtx->orientation = startDir;
  gameCtx->levelId = levelNum;
  GameContextLoadLevel(gameCtx, levelNum);
}

static void callbackSetGameFlag(EMCInterpreter *interp, uint16_t flag,
                                uint16_t val) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  if (val) {
    GameContextSetGameFlag(gameCtx, flag, val);
  } else {
    GameContextResetGameFlag(gameCtx, flag);
  }
}

static uint16_t callbackTestGameFlag(EMCInterpreter *interp, uint16_t flag) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return GameContextGetGameFlag(gameCtx, flag);
}

static void callbackLoadLevelGraphics(EMCInterpreter *interp,
                                      const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
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

static void callbackLoadBitmap(EMCInterpreter *interp, const char *file,
                               uint16_t param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(param == 2);
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));

  assert(CPSImageFromBuffer(&gameCtx->loadedbitMap, f.buffer, f.bufferSize));
}

static void callbackLoadDoorShapes(EMCInterpreter *interp, const char *file,
                                   uint16_t p1, uint16_t p2, uint16_t p3,
                                   uint16_t p4) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(p1 == 0);
  assert(p2 == 0);
  assert(p3 == 0);
  assert(p4 == 0);
  GameFile f;
  assert(GameEnvironmentGetFile(&f, file));
  assert(SHPHandleFromCompressedBuffer(&gameCtx->level->doors, f.buffer,
                                       f.bufferSize));
}

static void callbackLoadMonsterShapes(EMCInterpreter *interp, const char *file,
                                      uint16_t monsterId, uint16_t p2) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(monsterId < MAX_MONSTERS);
  assert(p2 == 0);
  GameFile f;
  assert(GameEnvironmentGetFile(&f, file));
  assert(SHPHandleFromBuffer(&gameCtx->level->monsterShapes[monsterId],
                             f.buffer, f.bufferSize));
}

static void callbackClearDialogField(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->dialogText = 0;
}

static uint16_t callbackCheckMonsterHostility(EMCInterpreter *interp,
                                              uint16_t monsterType) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("[UNIMPLEMENTED] callbackCheckMonsterHostility %x\n", monsterType);
  return 0;
  for (int i = 0; i < MAX_MONSTERS; i++) {
    if (gameCtx->level->monsters[i].type != monsterType &&
        monsterType != 0XFFFF) {
      continue;
    }
    return gameCtx->level->monsters[i].mode == 1 ? 0 : 1;
  }
  return 1;
}

static void callbackLoadMonster(EMCInterpreter *interp, uint16_t monsterId,
                                uint16_t shapeId, uint16_t hitChance,
                                uint16_t protection, uint16_t evadeChance,
                                uint16_t speed, uint16_t p6, uint16_t p7,
                                uint16_t p8) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("[INCOMPLETE] callbackLoadMonster %i %i\n", monsterId, shapeId);
  assert(monsterId < MAX_MONSTER_PROPERTIES);
  MonsterProperties *props = &gameCtx->level->monsterProperties[monsterId];
  props->shapeIndex = shapeId;
  props->fightingStats[0] = (hitChance << 8) / 100;
  props->fightingStats[1] = 256;
  props->fightingStats[2] = (protection << 8) / 100;
  props->fightingStats[3] = evadeChance;
  props->fightingStats[4] = (speed << 8) / 100;
  props->fightingStats[5] = (p6 << 8) / 100;
  props->fightingStats[6] = (p7 << 8) / 100;
  props->fightingStats[7] = (p8 << 8) / 100;
  props->fightingStats[8] = 0;
}

static void callbackLoadTimScript(EMCInterpreter *interp, uint16_t scriptId,
                                  const char *file) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameTimAnimatorLoadTim(&gameCtx->timAnimator, scriptId, file);
}

static void callbackRunTimScript(EMCInterpreter *interp, uint16_t scriptId,
                                 uint16_t loop) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
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
  GameTimAnimatorReleaseTim(&gameCtx->timAnimator, scriptId);
}

static uint16_t callbackGetItemInHand(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return gameCtx->itemInHand;
}

static uint16_t callbackGetItemParam(EMCInterpreter *interp, uint16_t itemId,
                                     EMCGetItemParam how) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  const Item *item = &gameCtx->itemsInGame[itemId];
  const ItemProperty *p = &gameCtx->itemProperties[item->itemPropertyIndex];
  switch (how) {
  case EMCGetItemParam_Block:
  case EMCGetItemParam_X:
  case EMCGetItemParam_Y:
  case EMCGetItemParam_LEVEL:
    return item->level;
  case EMCGetItemParam_PropertyIndex:
    return item->itemPropertyIndex;
  case EMCGetItemParam_CurrentFrameFlag:
  case EMCGetItemParam_NameStringId:
  case EMCGetItemParam_UNUSED_7:
  case EMCGetItemParam_ShpIndex:
  case EMCGetItemParam_Type:
  case EMCGetItemParam_ScriptFun:
  case EMCGetItemParam_Might:
  case EMCGetItemParam_Skill:
  case EMCGetItemParam_Protection:
  case EMCGetItemParam_14:
  case EMCGetItemParam_CurrentFrameFlag2:
  case EMCGetItemParam_Flags:
  case EMCGetItemParam_SkillMight:
    break;
  }
  assert(0);
  return 0;
}

static void callbackAllocItemProperties(EMCInterpreter *interp, uint16_t size) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->itemProperties = malloc(size * sizeof(ItemProperty));
  assert(gameCtx->itemProperties);
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

  gameCtx->itemProperties[index].shapeId = shapeId;
  gameCtx->itemProperties[index].stringId = stringId;
  uint8_t useLevelFile = 0;
  int realStringId = LangGetString(stringId, &useLevelFile);
  assert(useLevelFile == 0);

  LangHandleGetString(&gameCtx->lang, realStringId, c, sizeof(c));
}

static void callbackDisableControls(EMCInterpreter *interp, uint16_t mode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  assert(mode == 0);
  gameCtx->controlDisabled = 1;
}

static void callbackEnableControls(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->controlDisabled = 0;
}

static void callbackSetGlobalScriptVar(EMCInterpreter *interp, uint16_t index,
                                       uint16_t val) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->globalScriptVars[index] = val;
}

static uint16_t callbackGetGlobalScriptVar(EMCInterpreter *interp,
                                           uint16_t index) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return gameCtx->globalScriptVars[index];
}

static void callbackWSAInit(EMCInterpreter *interp, uint16_t index,
                            const char *wsaFile, int x, int y, int offscreen,
                            int flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameTimAnimatorWSAInit(&gameCtx->timAnimator, index, wsaFile, x, y, offscreen,
                         flags);
}

static void callbackCopyPage(EMCInterpreter *interp, uint16_t srcX,
                             uint16_t srcY, uint16_t destX, uint16_t destY,
                             uint16_t w, uint16_t h, uint16_t srcPage,
                             uint16_t dstPage) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackCopyPage x1=%i y1=%i x2=%i y2=%i w=%i h=%i "
         "scrPage=%i dstPage=%i\n",
         srcX, srcY, destX, destY, w, h, srcPage, dstPage);
  GameCopyPage(gameCtx, srcX, srcY, destX, destY, w, h, srcPage, dstPage);
}

static void callbackInitSceneDialog(EMCInterpreter *interp, int param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextSetState(gameCtx, GameState_GrowDialogBox);
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
  interp->callbacks.EMCInterpreterCallbacks_LoadDoorShapes =
      callbackLoadDoorShapes;
  interp->callbacks.EMCInterpreterCallbacks_LoadMonsterShapes =
      callbackLoadMonsterShapes;
  interp->callbacks.EMCInterpreterCallbacks_LoadMonster = callbackLoadMonster;
  interp->callbacks.EMCInterpreterCallbacks_ClearDialogField =
      callbackClearDialogField;
  interp->callbacks.EMCInterpreterCallbacks_CheckMonsterHostility =
      callbackCheckMonsterHostility;
  interp->callbacks.EMCInterpreterCallbacks_GetItemParam = callbackGetItemParam;
  interp->callbacks.EMCInterpreterCallbacks_EnableControls =
      callbackEnableControls;
  interp->callbacks.EMCInterpreterCallbacks_DisableControls =
      callbackDisableControls;
  interp->callbacks.EMCInterpreterCallbacks_LoadBitmap = callbackLoadBitmap;
  interp->callbacks.EMCInterpreterCallbacks_GetGlobalScriptVar =
      callbackGetGlobalScriptVar;
  interp->callbacks.EMCInterpreterCallbacks_SetGlobalScriptVar =
      callbackSetGlobalScriptVar;
  interp->callbacks.EMCInterpreterCallbacks_WSAInit = callbackWSAInit;
  interp->callbacks.EMCInterpreterCallbacks_InitSceneDialog =
      callbackInitSceneDialog;
  interp->callbacks.EMCInterpreterCallbacks_CopyPage = callbackCopyPage;
}
