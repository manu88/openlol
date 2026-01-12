#include "game_callbacks.h"
#include "formats/format_cps.h"
#include "formats/format_shp.h"
#include "formats/format_tim.h"
#include "game.h"
#include "game_ctx.h"
#include "game_envir.h"
#include "game_render.h"
#include "game_strings.h"
#include "game_tim_animator.h"
#include "geometry.h"
#include "logger.h"
#include "monster.h"
#include "render.h"
#include "script.h"
#include "ui.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_PREFIX "CALLBCK"

static uint16_t callbackRollDices(EMCInterpreter *interp, int16_t times,
                                  int16_t maxVal) {
  int res = 0;
  while (times--)
    res += GetRandom(1, maxVal);

  return res;
}

static uint16_t callbackGetDirection(EMCInterpreter *interp) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetDirection");
  assert(ctx);
  return ctx->orientation;
}

static int callbackCharacterSays(EMCInterpreter *interp, int16_t trackId,
                                 uint16_t charId, int redraw) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "characterSays trackId=0X%X charId=0X%X redraw=0X%X\n",
      trackId, charId, redraw);
  if (trackId == -1) {
    AudioSystemStopSpeech(&ctx->audio);
    return 1;
  }
  if (trackId == -2) {
    assert(0); // FIXME: implement me :)
  }
  GameContextPlayDialogSpeech(ctx, charId, trackId);
  return 1;
}

static void callbackPlayDialogue(EMCInterpreter *interp, int16_t charId,
                                 int16_t mode, uint16_t strId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  Log(LOG_PREFIX, "callbackPlayDialogue %x %x %x", charId, mode, strId);
  if (charId == 1) {
    charId = ctx->selectedChar;
  }
  GameContextSetDialogF(ctx, strId);
  GameContextPlayDialogSpeech(ctx, charId, strId);
}

static void callbackPrintMessage(EMCInterpreter *interp, uint16_t type,
                                 uint16_t strId, uint16_t soundId) {
  GameContext *ctx = (GameContext *)interp->callbackCtx;
  assert(ctx);
  Log(LOG_PREFIX, "callbackPrintMessage %x %x %x", type, strId, soundId);
  GameContextSetDialogF(ctx, strId);
  GameContextPlayDialogSpeech(ctx, 1, soundId);
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
    ctx->orientation = b;
    break;
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
  memset(gameCtx->level->blockProperties, 0, sizeof(BlockProperty) * 1024);
  MazeHandle mazHandle = {0};
  GameFile f = {0};
  assert(GameEnvironmentGetFile(&f, file));
  assert(MazeHandleFromBuffer(&mazHandle, f.buffer, f.bufferSize));

  for (int blockId = 0; blockId < 1024; blockId++) {
    const MazeBlock *block = mazHandle.maze->wallMappingIndices + blockId;

    for (int wallId = 0; wallId < 4; wallId++) {
      uint8_t wmi = block->face[wallId];
      gameCtx->level->blockProperties[blockId].walls[wallId] = wmi;
    }
    gameCtx->level->blockProperties[blockId].direction = 5;
#if 0
    if (_wllAutomapData[_levelBlockProperties[i].walls[0]] == 17) {
      _levelBlockProperties[i].flags &= 0xEF;
      _levelBlockProperties[i].flags |= 0x20;
    }
#endif
  }
  MazeHandleRelease(&mazHandle);
}

static void callbackSetWallType(EMCInterpreter *interp, uint16_t block,
                                uint16_t wall, uint16_t val) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackSetWallType %x %x %x", block, wall, val);
  if (wall == 0XFFFF) {
    for (int i = 0; i < 4; i++) {
      gameCtx->level->blockProperties[block].walls[i] = val;
    }
  } else {
    gameCtx->level->blockProperties[block].walls[wall] = val;
  }
}

static uint16_t callbackGetWallType(EMCInterpreter *interp, uint16_t blockId,
                                    uint16_t wall) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackGetWallType %x %x", blockId, wall);

  return gameCtx->level->blockProperties[blockId].walls[wall];
}

static void callbackLoadLevelShapes(EMCInterpreter *interp, const char *shpFile,
                                    const char *datFile) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackLoadLevelShapes %s %s", shpFile, datFile);

  GameContextLoadLevelShapes(gameCtx, shpFile, datFile);
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
    if (GameEnvironmentGetFileFromPak(&f, fileName, pakFile) == 0) {
      assert(GameEnvironmentGetFile(&f, fileName));
    }
    // assert(GameEnvironmentGetFileWithExt(&f, file, "VCN"));
    assert(VCNHandleFromLCWBuffer(&gameCtx->level->vcnHandle, f.buffer,
                                  f.bufferSize));

    gameCtx->animator.defaultPalette = gameCtx->level->vcnHandle.palette;
  }
  {
    GameFile f = {0};
    snprintf(fileName, 12, "%s.VMP", file);
    if (GameEnvironmentGetFileFromPak(&f, fileName, pakFile) == 0) {
      assert(GameEnvironmentGetFile(&f, fileName));
    }
    // assert(GameEnvironmentGetFileWithExt(&f, file, "VMP"));
    assert(VMPHandleFromLCWBuffer(&gameCtx->level->vmpHandle, f.buffer,
                                  f.bufferSize));
  }
  if (paletteFile) {
    GameFile f = {0};
    assert(GameEnvironmentGetFile(&f, paletteFile));
  }
  char fileName2[12] = "";
  snprintf(fileName, 12, "%s.SHP", file);
  snprintf(fileName2, 12, "%s.DAT", file);
  GameContextLoadLevelShapes(gameCtx, fileName, fileName2);
}

static void callbackRedrawPlayfield(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  gameCtx->showBitmap = 0;
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
  if (p1 != 0 || p2 != 0 || p3 != 0 || p4 != 0) {
    printf("FIXME: not supported yet\n");
  }
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
  assert(SHPHandleFromCompressedBuffer(
      &gameCtx->level->monsterShapes[monsterId], f.buffer, f.bufferSize));
}

static void callbackMoveParty(EMCInterpreter *interp, uint16_t how) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  if (how > 5 && how < 10) {
    how = (how - 6 - gameCtx->orientation) & 3;
  }
  switch (how) {
  case 0: // front
    GameContextButtonClicked(gameCtx, ButtonType_Up);
    break;
  case 1: // right
    GameContextButtonClicked(gameCtx, ButtonType_Right);
    break;
  case 2: // back
    GameContextButtonClicked(gameCtx, ButtonType_Down);
    break;
  case 3: // left
    GameContextButtonClicked(gameCtx, ButtonType_Left);
    break;
  case 4: // rotate left
    GameContextButtonClicked(gameCtx, ButtonType_TurnLeft);
    break;
  case 5: // rotate right
    GameContextButtonClicked(gameCtx, ButtonType_TurnRight);
    break;
  default:
    printf("callbackMoveParty unhandled how=%i\n", how);
    assert(0);
  }
}

static void callbackMoveMonster(EMCInterpreter *interp, uint16_t monsterId,
                                uint16_t destBlock, uint16_t xOff,
                                uint16_t yOff, uint16_t destDir) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Monster *monster = &gameCtx->level->monsters[monsterId];
  monster->block = destBlock;
  monster->destDirection = destDir;
  monster->destX = xOff;
  monster->destY = yOff;
}

static void callbackClearDialogField(EMCInterpreter *interp) {
  Log(LOG_PREFIX, "callbackClearDialogField");
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextResetDialog(gameCtx);
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
  Log(LOG_PREFIX, "callbackGetItemParam itemId=0X%x how=0X%x", itemId, how);
  const ItemProperty *p = &gameCtx->itemProperties[item->itemPropertyIndex];
  switch (how) {
  case EMCGetItemParam_LEVEL:
    return item->level;
  case EMCGetItemParam_PropertyIndex:
    return item->itemPropertyIndex;
  case EMCGetItemParam_CurrentFrameFlag:
    return item->shpCurFrame_flg;
  case EMCGetItemParam_NameStringId:
    return p->stringId;
  case EMCGetItemParam_UNUSED_7:
    assert(0);
  case EMCGetItemParam_ShpIndex:
    return p->shapeId;
  case EMCGetItemParam_Type:
    return p->type;
  case EMCGetItemParam_ScriptFun:
    return p->scriptFun;
  case EMCGetItemParam_Might:
    return p->might;
  case EMCGetItemParam_Skill:
    return p->skill;
  case EMCGetItemParam_Protection:
    return p->protection;
  case EMCGetItemParam_14:
    assert(0);
  case EMCGetItemParam_CurrentFrameFlag2:
    return item->shpCurFrame_flg & 0x1FFF;
  case EMCGetItemParam_Flags:
    return p->flags;
  case EMCGetItemParam_SkillMight:
    return (p->skill << 8) | ((uint8_t)p->might);
  case EMCGetItemParam_Block:
    return item->block;
  case EMCGetItemParam_X:
    return item->x;
  case EMCGetItemParam_Y:
    return item->y;
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

static uint16_t callbackGetWallFlags(EMCInterpreter *interp, uint16_t blockId,
                                     uint16_t wall) {
  Log(LOG_PREFIX, "callbackGetWallFlags %x %x", blockId, wall);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  uint8_t wmi = gameCtx->level->blockProperties[blockId].walls[wall];
  const WllWallMapping *mapping =
      WllHandleGetWallMapping(&gameCtx->level->wllHandle, wmi);
  return mapping->flags;
}

static void callbackFadeScene(EMCInterpreter *interp, uint16_t mode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("callbackFadeScene mode=0X%X\n", mode);
  GameDoSceneFade(gameCtx, 10);
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

static int callbackCheckMagic(EMCInterpreter *interp, uint16_t charId,
                              uint16_t spellNum, uint16_t spellLevel) {
  Log(LOG_PREFIX,
      "callbackCheckMagic charId=0X%X spellNum=0X%X spellLevel=0X%X\n ", charId,
      spellNum, spellLevel);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return GameContextCheckMagic(gameCtx, charId, spellNum, spellLevel);
}

static uint16_t callbackCreateLevelItem(EMCInterpreter *interp,
                                        uint16_t itemType, uint16_t frame,
                                        uint16_t flags, uint16_t level,
                                        uint16_t block, uint16_t xOff,
                                        uint16_t yOff, uint16_t flyingHeight) {
  Log(LOG_PREFIX,
      "[INCOMPLETE] callbackCreateLevelItem itemType=0X%X frame=0X%X "
      "flags=0X%X\n "
      "level=%i block=0X%X xOff=0X%X yOff=0X%X flyingHeight=0X%X\n",
      itemType, frame, flags, level, block, xOff, yOff, flyingHeight);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  uint16_t index = GameContextCreateItem(gameCtx, itemType);
  return index;
}

static uint16_t callbackProcessDialog(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  Log(LOG_PREFIX, "callbackProcessDialog\n");
  return gameCtx->dialogState == DialogState_Done;
}

static void callbackPlaySoundFX(EMCInterpreter *interp, uint16_t soundId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextPlaySoundFX(gameCtx, soundId);
}

static void callbackCharacterSurpriseSFX(EMCInterpreter *interp) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  for (int i = 0; i < 4; i++) {
    if (!(gameCtx->chars[i].flags & 1) || gameCtx->chars[i].id >= 0)
      continue;
    int s = -gameCtx->chars[i].id;
    int sfx = (s == 1)
                  ? 136
                  : ((s == 5) ? 50 : ((s == 8) ? 49 : ((s == 9) ? 48 : 0)));
    if (sfx) {
      GameContextPlaySoundFX(gameCtx, sfx);
    }
    break;
  }
}

static void callbackPrintWindowText(EMCInterpreter *interp, uint16_t dim,
                                    uint16_t flags, uint16_t stringId) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  char *text = GameContextGetString3(gameCtx, stringId);
  UISetStyle(UIStyle_Inventory);
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, MAZE_COORDS_X + 10,
               MAZE_COORDS_Y + 20, MAZE_COORDS_W - 20, text);
  free(text);
}

static int callbackInitMonster(EMCInterpreter *interp, uint16_t block,
                               uint16_t xOff, uint16_t yOff,
                               uint16_t orientation, uint16_t monsterType,
                               uint16_t flags, uint16_t monsterMode) {
  Log(LOG_PREFIX,
      "callbackInitMonster block=0X%X xOff=0X%X yOff=0X%X orientation=0X%X "
      "monsterType=0X%X flags=0X%X monsterMode=0X%X \n",
      block, xOff, yOff, orientation, monsterType, flags, monsterMode);
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  int index = MonsterGetAvailableSlot(gameCtx);
  if (index == -1) {
    return -1;
  }
  Monster *monster = &gameCtx->level->monsters[index];
  MonsterInit(monster);
  monster->available = 0;
  monster->id = index;
  printf("InitMonster: using index %i\n", index);
  monster->block = block;
  monster->orientation = orientation;
  monster->type = monsterType;
  monster->flags = flags;
  return index;
}

static uint16_t callbackGetMonsterStat(EMCInterpreter *interp,
                                       uint16_t monsterId,
                                       GetMonsterStatHow how) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  const Monster *monster = gameCtx->level->monsters + monsterId;
  const MonsterProperties *props =
      gameCtx->level->monsterProperties + monster->type;
  switch (how) {
  case GetMonsterStatHow_Mode:
    return monster->mode;
  case GetMonsterStatHow_HitPoints:
    return monster->hitPoints;
  case GetMonsterStatHow_Block:
    return monster->block;
  case GetMonsterStatHow_Facing:
    return monster->orientation;
  case GetMonsterStatHow_Type:
    return monster->type;
  case GetMonsterStatHow_PropertyHitPoint:
    return props->hitPoints;
  case GetMonsterStatHow_Flags:
    return monster->flags;
  case GetMonsterStatHow_PropertyFlags:
    return props->flags;
  case GetMonsterStatHow_AnimType:
    return 0;
  default:
    assert(0);
  }
  return 0;
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

static int callbackTriggerEventOnMouseButtonClick(EMCInterpreter *interp,
                                                  uint16_t event) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  return GameWaitForClick(gameCtx);
}

static void
callbackPrepareSpecialScene(EMCInterpreter *interp, uint16_t fieldType,
                            uint16_t hasDialogue, uint16_t suspendGUI,
                            uint16_t allowSceneUpdate, uint16_t controlMode,
                            uint16_t fadeFlag) {
  printf("[UNIMPLEMENTED] prepareSpecialScene fieldType %X hasDialogue %X "
         "suspendGUI %X "
         "allowSceneUpdate %X controlMode %X  fadeFlag %X \n",
         fieldType, hasDialogue, suspendGUI, allowSceneUpdate, controlMode,
         fadeFlag);
}

static void callbackRestoreAfterSpecialScene(EMCInterpreter *interp,
                                             uint16_t fadeFlag,
                                             uint16_t redrawPlayField,
                                             uint16_t releaseTimScripts,
                                             uint16_t sceneUpdateMode) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  printf("[unimplemented] restoreAfterSpecialScene fadeFlag=%X "
         "redrawPlayField=%X releaseTimScripts=%X sceneUpdateMode=%X\n",
         fadeFlag, redrawPlayField, releaseTimScripts, sceneUpdateMode);
  gameCtx->showBitmap = 0;
}

void GameContextInstallCallbacks(EMCInterpreter *interp) {
  interp->callbacks.EMCInterpreterCallbacks_RollDices = callbackRollDices;
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
  interp->callbacks.EMCInterpreterCallbacks_MoveMonster = callbackMoveMonster;
  interp->callbacks.EMCInterpreterCallbacks_PlaySoundFX = callbackPlaySoundFX;
  interp->callbacks.EMCInterpreterCallbacks_CharacterSurpriseSFX =
      callbackCharacterSurpriseSFX;
  interp->callbacks.EMCInterpreterCallbacks_MoveParty = callbackMoveParty;
  interp->callbacks.EMCInterpreterCallbacks_FadeScene = callbackFadeScene;

  interp->callbacks.EMCInterpreterCallbacks_PrepareSpecialScene =
      callbackPrepareSpecialScene;
  interp->callbacks.EMCInterpreterCallbacks_RestoreAfterSpecialScene =
      callbackRestoreAfterSpecialScene;
  interp->callbacks.EMCInterpreterCallbacks_InitMonster = callbackInitMonster;
  interp->callbacks.EMCInterpreterCallbacks_CreateLevelItem =
      callbackCreateLevelItem;

  interp->callbacks.EMCInterpreterCallbacks_GetMonsterStat =
      callbackGetMonsterStat;
  interp->callbacks.EMCInterpreterCallbacks_PrintWindowText =
      callbackPrintWindowText;
  interp->callbacks.EMCInterpreterCallbacks_RedrawPlayfield =
      callbackRedrawPlayfield;
  interp->callbacks.EMCInterpreterCallbacks_TriggerEventOnMouseButtonClick =
      callbackTriggerEventOnMouseButtonClick;
  interp->callbacks.EMCInterpreterCallbacks_CharacterSays =
      callbackCharacterSays;
  interp->callbacks.EMCInterpreterCallbacks_CheckMagic = callbackCheckMagic;
}
