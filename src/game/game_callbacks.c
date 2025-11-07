#include "game_callbacks.h"
#include "formats/format_cps.h"
#include "formats/format_shp.h"
#include "formats/format_tim.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "game_tim_animator.h"
#include "logger.h"
#include "render.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_PREFIX "CALLBCK"

static uint16_t callbackGetDirection(EMCInterpreter *interp) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetDirection");
  assert(ctx);
  return ctx->orientation;
}

static void callbackPlayDialogue(EMCInterpreter *interp, int16_t charId,
                                 int16_t mode, uint16_t strId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  Log(LOG_PREFIX, "callbackPlayDialogue %x %x %x", charId, mode, strId);
  GameRenderSetDialogF(ctx, strId);
}

static void callbackPrintMessage(EMCInterpreter *interp, uint16_t type,
                                 uint16_t strId, uint16_t soundId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  Log(LOG_PREFIX, "callbackPrintMessage %x %x %x", type, strId, soundId);
  GameRenderSetDialogF(ctx, strId);
}

static uint16_t callbackGetGlobalVar(EMCInterpreter *interp, EMCGlobalVarID id,
                                     uint16_t a) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  Log(LOG_PREFIX, "callbackGetGlobalVar %x %x", id, a);
  switch (id) {
  case EMCGlobalVarID_CurrentBlock:
    return ctx->currentBock;
  case EMCGlobalVarID_CurrentDir:
    return ctx->orientation;
  case EMCGlobalVarID_CurrentLevel:
  case EMCGlobalVarID_ItemInHand:
  case EMCGlobalVarID_Brightness:
  case EMCGlobalVarID_Credits:
    return ctx->credits;
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
  Log(LOG_PREFIX, "callbackSetGlobalVar %x %x %x", id, a, b);
  switch (id) {
  case EMCGlobalVarID_CurrentBlock: {
    ctx->currentBock = b;
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
  Log(LOG_PREFIX, "callbackLoadLangFile %s", file);
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
  Log(LOG_PREFIX, "callbackLoadCMZ %s", file);
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));
  assert(
      MazeHandleFromBuffer(&gameCtx->level->mazHandle, f.buffer, f.bufferSize));
}

static void callbackLoadLevelShapes(EMCInterpreter *interp, const char *shpFile,
                                    const char *datFile) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackLoadLevelShapes %s %s", shpFile, datFile);
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
  Log(LOG_PREFIX, "callbackLoadLevel %x %x %x", levelNum, startBlock, startDir);
  gameCtx->currentBock = startBlock;
  gameCtx->orientation = startDir;
  gameCtx->levelId = levelNum;
  GameContextLoadLevel(gameCtx, levelNum);
}

static void callbackSetGameFlag(EMCInterpreter *interp, uint16_t flag,
                                uint16_t set) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetGameFlag %x %x", flag, set);
  if (set) {
    GameContextSetGameFlag(gameCtx, flag);
  } else {
    GameContextResetGameFlag(gameCtx, flag);
  }
}

static uint16_t callbackTestGameFlag(EMCInterpreter *interp, uint16_t flag) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackTestGameFlag %x", flag);
  return GameContextGetGameFlag(gameCtx, flag);
}

static void callbackLoadLevelGraphics(EMCInterpreter *interp, const char *file,
                                      const char *paletteFile) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackLoadLevelGraphics %s %s", file, paletteFile);
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

    gameCtx->animator.defaultPalette = gameCtx->level->vcnHandle.palette;
  }
  {
    GameFile f = {0};
    snprintf(fileName, 12, "%s.VMP", file);
    assert(GameEnvironmentGetFileFromPak(&f, fileName, pakFile));
    // assert(GameEnvironmentGetFileWithExt(&f, file, "VMP"));
    assert(VMPHandleFromLCWBuffer(&gameCtx->level->vmpHandle, f.buffer,
                                  f.bufferSize));
  }
  if (paletteFile) {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, paletteFile));
  }
}

static void callbackLoadBitmap(EMCInterpreter *interp, const char *file,
                               uint16_t param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackLoadBitmap %s %x", file, param);
  assert(param == 2);
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));

  assert(CPSImageFromBuffer(&gameCtx->loadedbitMap, f.buffer, f.bufferSize));
}

static void callbackLoadDoorShapes(EMCInterpreter *interp, const char *file,
                                   uint16_t p1, uint16_t p2, uint16_t p3,
                                   uint16_t p4) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackLoadDoorShapes %s %x %x %x %x", file, p1, p2, p3,
      p4);
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
  Log(LOG_PREFIX, "callbackLoadMonsterShapes %s %x %x", file, monsterId, p2);
  assert(monsterId < MAX_MONSTERS);
  assert(p2 == 0);
  GameFile f;
  assert(GameEnvironmentGetFile(&f, file));
  assert(SHPHandleFromBuffer(&gameCtx->level->monsterShapes[monsterId],
                             f.buffer, f.bufferSize));
}

static void callbackClearDialogField(EMCInterpreter *interp) {
  Log(LOG_PREFIX, "callbackClearDialogField");
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameRenderResetDialog(gameCtx);
}

static uint16_t callbackCheckMonsterHostility(EMCInterpreter *interp,
                                              uint16_t monsterType) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, " UNIMPLEMENTED callbackCheckMonsterHostility %x",
      monsterType);
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
  Log(LOG_PREFIX, " INCOMPLETE callbackLoadMonster %i %i\n", monsterId,
      shapeId);
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
  Log(LOG_PREFIX, "callbackLoadTimScript %x %s", scriptId, file);
  GameTimInterpreterLoadTim(&gameCtx->timInterpreter, scriptId, file);
}

static void callbackRunTimScript(EMCInterpreter *interp, uint16_t scriptId,
                                 uint16_t loop) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackRunTimScript %x %i", scriptId, loop);
  if (!gameCtx->timInterpreter.tim[scriptId].avtl) {
    printf("NO TIM loaded\n");
    return;
  }
  GameContextSetState(gameCtx, GameState_TimAnimation);
  GameTimInterpreterRunTim(&gameCtx->timInterpreter, scriptId);
}

static void callbackReleaseTimScript(EMCInterpreter *interp,
                                     uint16_t scriptId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackReleaseTimScript %i", scriptId);
  GameTimInterpreterReleaseTim(&gameCtx->timInterpreter, scriptId);
}

static uint16_t callbackGetItemIndexInHand(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetItemIndexInHand");
  return gameCtx->itemIndexInHand;
}

static uint16_t callbackGetItemParam(EMCInterpreter *interp, uint16_t itemId,
                                     EMCGetItemParam how) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  const GameObject *item = &gameCtx->itemsInGame[itemId];
  Log(LOG_PREFIX, "callbackGetItemParam %x %x", itemId, how);
  // const ItemProperty *p = &gameCtx->itemProperties[item->itemPropertyIndex];
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
  Log(LOG_PREFIX, "callbackAllocItemProperties %i", size);
  gameCtx->itemProperties = malloc(size * sizeof(ItemProperty));
  assert(gameCtx->itemProperties);
  gameCtx->itemsCount = size;
}

static void callbackSetItemProperty(EMCInterpreter *interp, uint16_t index,
                                    uint16_t stringId, uint16_t shapeId,
                                    uint16_t type, uint16_t scriptFun,
                                    uint16_t might, uint16_t skill,
                                    uint16_t protection, uint16_t flags) {

  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetItemProperty %x %x %x %x %x %x %x %x %x", index,
      stringId, shapeId, type, scriptFun, might, skill, protection, flags);
  assert(index < gameCtx->itemsCount);

  gameCtx->itemProperties[index].shapeId = shapeId;
  gameCtx->itemProperties[index].stringId = stringId;
  gameCtx->itemProperties[index].type = type;
  gameCtx->itemProperties[index].scriptFun = scriptFun;
  gameCtx->itemProperties[index].might = might;
  gameCtx->itemProperties[index].skill = skill;
  gameCtx->itemProperties[index].protection = protection;
  gameCtx->itemProperties[index].flags = flags;
}

static void callbackDisableControls(EMCInterpreter *interp, uint16_t mode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackDisableControls %x", mode);
  assert(mode == 0);
  gameCtx->controlDisabled = 1;
}

static void callbackEnableControls(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackEnableControls");
  gameCtx->controlDisabled = 0;
}

static void callbackSetGlobalScriptVar(EMCInterpreter *interp, uint16_t index,
                                       uint16_t val) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetGlobalScriptVar %x %x", index, val);
  gameCtx->globalScriptVars[index] = val;
}

static uint16_t callbackGetGlobalScriptVar(EMCInterpreter *interp,
                                           uint16_t index) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetGlobalScriptVar %x", index);
  return gameCtx->globalScriptVars[index];
}

static void callbackWSAInit(EMCInterpreter *interp, uint16_t index,
                            const char *wsaFile, int x, int y, int offscreen,
                            int flags) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackWSAInit %x %s %i %i %i %i", index, wsaFile, x, y,
      offscreen, flags);
  GameFile f = {0};
  printf("----> GameTimAnimator load wsa file '%s' index %i offscreen=%i\n",
         wsaFile, index, offscreen);
  assert(GameEnvironmentGetFileWithExt(&f, wsaFile, "WSA"));
  AnimatorInitWSA(&gameCtx->animator, f.buffer, f.bufferSize, x, y, offscreen,
                  flags);
}

static void callbackRestoreAfterSceneDialog(EMCInterpreter *interp, int mode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackRestoreAfterSceneDialog %x", mode);
  GameContextCleanupSceneDialog(gameCtx);
}

static void callbackRestoreAfterSceneWindowDialog(EMCInterpreter *interp,
                                                  int redraw) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackRestoreAfterSceneWindowDialog %x", redraw);
  GameContextCleanupSceneDialog(gameCtx);
}

static void callbackSetWallType(EMCInterpreter *interp, uint16_t p0,
                                uint16_t p1, uint16_t p2) {
  Log(LOG_PREFIX, "callbackSetWallType %x %x %x", p0, p1, p2);
}

static uint16_t callbackCheckForCertainPartyMember(EMCInterpreter *interp,
                                                   uint16_t charId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    if (abs(gameCtx->chars[i].id) == charId) {
      return 1;
    }
  }
  return 0;
}

static void callbackSetNextFunc(EMCInterpreter *interp, uint16_t func) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetNextFunc %x", func);
  assert(gameCtx->nextFunc == 0);
  gameCtx->nextFunc = func;
}

static uint16_t callbackGetWallType(EMCInterpreter *interp, uint16_t index,
                                    uint16_t index2) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetWallType %x %x", index, index2);
  const MazeBlock *block =
      gameCtx->level->mazHandle.maze->wallMappingIndices + index;
  uint8_t wmi = block->face[index2];
  return wmi;
}

static uint16_t callbackGetWallFlags(EMCInterpreter *interp, uint16_t index,
                                     uint16_t index2) {
  Log(LOG_PREFIX, "callbackGetWallFlags %x %x", index, index2);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  const MazeBlock *block =
      gameCtx->level->mazHandle.maze->wallMappingIndices + index;
  uint8_t wmi = block->face[index2];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  return mapping->flags;
}

static void callbackSetupDialogueButtons(EMCInterpreter *interp,
                                         uint16_t numStrs, uint16_t strIds[3]) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetupDialogueButtons %x %x %x %x", numStrs,
      strIds[0], strIds[1], strIds[2]);
  gameCtx->dialogState = DialogState_InProgress;
  GameContextInitSceneDialog(gameCtx);
  printf("callbackSetupDialogueButtons %i %X %X %X\n", numStrs, strIds[0],
         strIds[1], strIds[2]);
  for (int i = 0; i < numStrs; i++) {
    assert(strIds[i] != 0XFFFF);
    gameCtx->buttonText[i] = malloc(16);
    assert(gameCtx->buttonText[i]);
    memset(gameCtx->buttonText[i], 0, 16);
    GameContextGetString(gameCtx, strIds[i], gameCtx->buttonText[i], 16);
  }
}

static void callbackSetupBackgroundAnimationPart(
    EMCInterpreter *interp, uint16_t animIndex, uint16_t part,
    uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles, uint16_t nextPart,
    uint16_t partDelay, uint16_t field, uint16_t sfxIndex, uint16_t sfxFrame) {

  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX,
      "callbackSetupBackgroundAnimationPart animIndex=%x part=%x firstFrame=%x "
      "lastFrame=%x cycles=%x nextPart=%x partDelay=%x field=%x sfxIndex=%x "
      "sfxFrame%x\n",
      animIndex, part, firstFrame, lastFrame, cycles, nextPart, partDelay,
      field, sfxIndex, sfxFrame);

  AnimatorSetupPart(gameCtx->timInterpreter.animator, animIndex, part,
                    firstFrame, lastFrame, cycles, nextPart, partDelay, field,
                    sfxIndex, sfxFrame);
}

static void callbackDeleteHandItem(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackDeleteHandItem");
  GameContextDeleteItem(gameCtx, gameCtx->itemIndexInHand);
  gameCtx->itemIndexInHand = 0;
  GameContextUpdateCursor(gameCtx);
}

static uint16_t callbackCreateHandItem(EMCInterpreter *interp,
                                       uint16_t itemType, uint16_t p1,
                                       uint16_t p2) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackCreateHandItem");
  if (gameCtx->itemIndexInHand) {
    return 0;
  }
  gameCtx->itemIndexInHand = GameContextCreateItem(gameCtx, itemType);
  uint16_t frameId =
      itemType ? GameContextGetItemSHPFrameIndex(gameCtx, itemType) : 0;
  createCursorForItem(gameCtx, frameId);
  return 1;
}

static uint16_t callbackProcessDialog(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackProcessDialog\n");
  return gameCtx->dialogState == DialogState_Done;
}

static uint16_t callbackCheckRectForMousePointer(EMCInterpreter *interp,
                                                 uint16_t xMin, uint16_t yMin,
                                                 uint16_t xMax, uint16_t yMax) {
  Log(LOG_PREFIX, "callbackCheckRectForMousePointer %i %i %i %i\n", xMin, yMin,
      xMax, yMax);
  int x;
  int y;
  SDL_GetMouseState(&x, &y);
  x /= SCREEN_FACTOR;
  y /= SCREEN_FACTOR;

  uint16_t ret = x >= xMin && x < xMax && y >= yMin && y < yMax;
  return ret;
}

static void callbackDrawExitButton(EMCInterpreter *interp, uint16_t p0,
                                   uint16_t p1) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackDrawExitButton %x %x", p0, p1);
  gameCtx->drawExitSceneButton = 1;
  gameCtx->exitSceneButtonDisabled = 1;
}

static void callbackCopyPage(EMCInterpreter *interp, uint16_t srcX,
                             uint16_t srcY, uint16_t destX, uint16_t destY,
                             uint16_t w, uint16_t h, uint16_t srcPage,
                             uint16_t dstPage) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX,
      "callbackCopyPage x1=%i y1=%i x2=%i y2=%i w=%i h=%i "
      "scrPage=%i dstPage=%i\n",
      srcX, srcY, destX, destY, w, h, srcPage, dstPage);
  GameCopyPage(gameCtx, srcX, srcY, destX, destY, w, h, srcPage, dstPage);
}

static void callbackInitSceneDialog(EMCInterpreter *interp, int param) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackInitSceneDialog %x", param);
  GameContextInitSceneDialog(gameCtx);
}

static void callbackPlayAnimationPart(EMCInterpreter *interp,
                                      uint16_t animIndex, uint16_t firstFrame,
                                      uint16_t lastFrame, uint16_t delay) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX,
      "callbackPlayAnimationPart animIndex=%x firstFrame=%x lastFrame=%x "
      "delay=%x\n",
      animIndex, firstFrame, lastFrame, delay);
  GameContextSetState(gameCtx, GameState_TimAnimation);
  AnimatorPlayPart(gameCtx->timInterpreter.animator, animIndex, firstFrame,
                   lastFrame, delay);
}

static uint16_t callbackGetCredits(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return gameCtx->credits;
}

static void callbackCreditsTransaction(EMCInterpreter *interp, int16_t amount) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->credits += amount;
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
  interp->callbacks.EMCInterpreterCallbacks_GetItemIndexInHand =
      callbackGetItemIndexInHand;
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
  interp->callbacks.EMCInterpreterCallbacks_DrawExitButton =
      callbackDrawExitButton;
  interp->callbacks.EMCInterpreterCallbacks_RestoreAfterSceneDialog =
      callbackRestoreAfterSceneDialog;
  interp->callbacks.EMCInterpreterCallbacks_RestoreAfterSceneWindowDialog =
      callbackRestoreAfterSceneWindowDialog;
  interp->callbacks.EMCInterpreterCallbacks_GetWallType = callbackGetWallType;
  interp->callbacks.EMCInterpreterCallbacks_SetWallType = callbackSetWallType;
  interp->callbacks.EMCInterpreterCallbacks_GetWallFlags = callbackGetWallFlags;
  interp->callbacks.EMCInterpreterCallbacks_CheckRectForMousePointer =
      callbackCheckRectForMousePointer;
  interp->callbacks.EMCInterpreterCallbacks_SetupDialogueButtons =
      callbackSetupDialogueButtons;
  interp->callbacks.EMCInterpreterCallbacks_ProcessDialog =
      callbackProcessDialog;
  interp->callbacks.EMCInterpreterCallbacks_SetupBackgroundAnimationPart =
      callbackSetupBackgroundAnimationPart;
  interp->callbacks.EMCInterpreterCallbacks_DeleteHandItem =
      callbackDeleteHandItem;
  interp->callbacks.EMCInterpreterCallbacks_PlayAnimationPart =
      callbackPlayAnimationPart;
  interp->callbacks.EMCInterpreterCallbacks_CreateHandItem =
      callbackCreateHandItem;
  interp->callbacks.EMCInterpreterCallbacks_CheckForCertainPartyMember =
      callbackCheckForCertainPartyMember;
  interp->callbacks.EMCInterpreterCallbacks_SetNextFunc = callbackSetNextFunc;
  interp->callbacks.EMCInterpreterCallbacks_GetCredits = callbackGetCredits;
  interp->callbacks.EMCInterpreterCallbacks_CreditsTransaction =
      callbackCreditsTransaction;
}
