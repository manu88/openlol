#include "script_builtins.h"
#include "game_ctx.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ASSERT_UNIMPLEMENTED assert(0)

static uint16_t allocItemProperties(EMCInterpreter *interp, EMCState *state) {
  uint16_t size = EMCStateStackVal(state, 0);
  interp->callbacks.EMCInterpreterCallbacks_AllocItemProperties(interp, size);
  return 1;
}

static uint16_t setItemProperty(EMCInterpreter *interp, EMCState *state) {
  uint16_t index = EMCStateStackVal(state, 0);
  uint16_t stringId = EMCStateStackVal(state, 1);
  uint16_t shpId = EMCStateStackVal(state, 2);
  uint16_t type = EMCStateStackVal(state, 3);
  uint16_t scriptFun = EMCStateStackVal(state, 4);
  uint16_t might = EMCStateStackVal(state, 5);
  uint16_t skill = EMCStateStackVal(state, 6);
  uint16_t protection = EMCStateStackVal(state, 7);
  uint16_t flags = EMCStateStackVal(state, 8);
  interp->callbacks.EMCInterpreterCallbacks_SetItemProperty(
      interp, index, stringId, shpId, type, scriptFun, might, skill, protection,
      flags);
  return 1;
}

static uint16_t getWallType(EMCInterpreter *interp, EMCState *state) {
  printf("getWallType\n");
  ASSERT_UNIMPLEMENTED;
  return 1;
}
static uint16_t drawScene(EMCInterpreter *interp, EMCState *state) {
  int16_t pageNum = EMCStateStackVal(state, 0);
  printf("drawScene %X\n", pageNum);
  return 1;
}
static uint16_t rollDice(EMCInterpreter *interp, EMCState *state) {
  int16_t times = EMCStateStackVal(state, 0);
  int16_t max = EMCStateStackVal(state, 1);
  printf("rollDice times=%i max=%i\n", times, max);
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t enableSysTimer(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] enableSysTimer\n");
  // ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t initDialogueSequence(EMCInterpreter *interp, EMCState *state) {
  GameContext *gameCtx = (GameContext *)interp->callbackCtx;
  GameContextSetState(gameCtx, GameState_GrowDialogBox);
  return 1;
}

static uint16_t restoreAfterDialogueSequence(EMCInterpreter *interp,
                                             EMCState *state) {
  printf("restoreAfterDialogueSequence\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t resetPortraitsAndDisableSysTimer(EMCInterpreter *interp,
                                                 EMCState *state) {
  printf("[UNIMPLEMENTED] enableSysTimer\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t setGameFlag(EMCInterpreter *interp, EMCState *state) {
  uint16_t flag = EMCStateStackVal(state, 0);
  uint16_t val = EMCStateStackVal(state, 1);
  interp->callbacks.EMCInterpreterCallbacks_SetGameFlag(interp, flag, val);
  return 1;
}

static uint16_t testGameFlag(EMCInterpreter *interp, EMCState *state) {
  uint16_t p = EMCStateStackVal(state, 0);
  return interp->callbacks.EMCInterpreterCallbacks_TestGameFlag(interp, p);
}

static uint16_t getItemParam(EMCInterpreter *interp, EMCState *state) {
  uint16_t p0 = EMCStateStackVal(state, 0);
  uint16_t p1 = EMCStateStackVal(state, 1);
  if (p0 == 0) {
    return 0;
  }
  return interp->callbacks.EMCInterpreterCallbacks_GetItemParam(
      interp, p0, (EMCGetItemParam)p1);
}

static uint16_t createLevelItem(EMCInterpreter *interp, EMCState *state) {
  printf("createLevelItem\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t playAnimationPart(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] playAnimationPart\n");
  // ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t setNextFunc(EMCInterpreter *interp, EMCState *state) {
  uint16_t p = EMCStateStackVal(state, 0);
  printf("setNextFunc %x\n", p);
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t getDirection(EMCInterpreter *interp, EMCState *state) {
  return interp->callbacks.EMCInterpreterCallbacks_GetDirection(interp);
}

static uint16_t checkRectForMousePointer(EMCInterpreter *interp,
                                         EMCState *state) {
  printf("checkRectForMousePointer\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}
static uint16_t clearDialogueField(EMCInterpreter *interp, EMCState *state) {
  interp->callbacks.EMCInterpreterCallbacks_ClearDialogField(interp);
  return 1;
}
static uint16_t setupBackgroundAnimationPart(EMCInterpreter *interp,
                                             EMCState *state) {
  printf("setupBackgroundAnimationPart\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}
static uint16_t hideMouse(EMCInterpreter *interp, EMCState *state) {
  printf("hideMouse\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}
static uint16_t showMouse(EMCInterpreter *interp, EMCState *state) {
  printf("showMouse\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}
static uint16_t fadeToBlack(EMCInterpreter *interp, EMCState *state) {
  printf("fadeToBlack\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}
static uint16_t loadBitmap(EMCInterpreter *interp, EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  int16_t p1 = EMCStateStackVal(state, 1);
  const char *f = EMCStateGetDataString(state, p0);
  interp->callbacks.EMCInterpreterCallbacks_LoadBitmap(interp, f, p1);
  return 1;
}
static uint16_t stopBackgroundAnimation(EMCInterpreter *interp,
                                        EMCState *state) {
  printf("stopBackgroundAnimation\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

void calcCoordinates(uint16_t *x, uint16_t *y, int block, uint16_t xOffs,
                     uint16_t yOffs) {
  *x = (block & 0x1F) << 8 | xOffs;
  *y = ((block & 0xFFE0) << 3) | yOffs;
}

static uint16_t getGlobalVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t how = EMCStateStackVal(state, 0);
  uint16_t a = EMCStateStackVal(state, 1);
  return interp->callbacks.EMCInterpreterCallbacks_GetGlobalVar(interp, how, a);
}

static uint16_t initMonster(EMCInterpreter *interp, EMCState *state) {
  printf("initMonster\n");
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t setGlobalVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t how = EMCStateStackVal(state, 0);
  uint16_t a = EMCStateStackVal(state, 1);
  uint16_t b = EMCStateStackVal(state, 2);
  return interp->callbacks.EMCInterpreterCallbacks_SetGlobalVar(interp, how, a,
                                                                b);
}

static uint16_t loadSoundFile(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] loadSoundFile\n");
  return 1;
}

static uint16_t playMusicTrack(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] playMusicTrack\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t fadeClearSceneWindow(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] fadeClearSceneWindow\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t fadeSequencePalette(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] fadeSequencePalette\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t initSceneWindowDialogue(EMCInterpreter *interp,
                                        EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  interp->callbacks.EMCInterpreterCallbacks_InitSceneDialog(interp, p0);
  return 1;
}

static uint16_t startBackgroundAnimation(EMCInterpreter *interp,
                                         EMCState *state) {
  printf("startBackgroundAnimation\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t copyRegion(EMCInterpreter *interp, EMCState *state) {
  uint16_t srcX = EMCStateStackVal(state, 0);
  uint16_t srcY = EMCStateStackVal(state, 1);
  uint16_t destX = EMCStateStackVal(state, 2);
  uint16_t destY = EMCStateStackVal(state, 3);
  uint16_t w = EMCStateStackVal(state, 4);
  uint16_t h = EMCStateStackVal(state, 5);
  uint16_t srcPage = EMCStateStackVal(state, 6);
  uint16_t dstPage = EMCStateStackVal(state, 6);
  interp->callbacks.EMCInterpreterCallbacks_CopyPage(
      interp, srcX, srcY, destX, destY, w, h, srcPage, dstPage);
  return 1;
}
static uint16_t moveMonster(EMCInterpreter *interp, EMCState *state) {
  printf("moveMonster\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t loadTimScript(EMCInterpreter *interp, EMCState *state) {
  uint16_t scriptId = EMCStateStackVal(state, 0);
  uint16_t stringId = EMCStateStackVal(state, 1);
  const char *str = EMCStateGetDataString(state, stringId);
  interp->callbacks.EMCInterpreterCallbacks_LoadTimScript(interp, scriptId,
                                                          str);
  return 1;
}

static uint16_t runTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  int16_t loop = EMCStateStackVal(state, 1);
  interp->callbacks.EMCInterpreterCallbacks_RunTimScript(interp, scriptId,
                                                         loop);
  return 1;
}

static uint16_t stopTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  printf("[UNIMPLEMENTED] stopTimScript scriptId=%X\n", scriptId);
  return 1;
}

static uint16_t releaseTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  interp->callbacks.EMCInterpreterCallbacks_ReleaseTimScript(interp, scriptId);
  return 1;
}
static uint16_t getItemInHand(EMCInterpreter *interp, EMCState *state) {
  return interp->callbacks.EMCInterpreterCallbacks_GetItemInHand(interp);
}
static uint16_t playSoundEffect(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] playSoundEffect\n");
  return 0;
}

static uint16_t playCharacterScriptChat(EMCInterpreter *interp,
                                        EMCState *state) {
  int16_t charId = EMCStateStackVal(state, 0);
  int16_t mode = EMCStateStackVal(state, 1);
  int16_t stringId = EMCStateStackVal(state, 2);
  interp->callbacks.EMCInterpreterCallbacks_PlayDialogue(interp, charId, mode,
                                                         stringId);
  return 1;
}

static uint16_t drawExitButton(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] drawExitButton\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t triggerEventOnMouseButtonClick(EMCInterpreter *interp,
                                               EMCState *state) {
  int evt = EMCStateStackVal(state, 0);
  printf("triggerEventOnMouseButtonClick evt=%X\n", evt);
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t printMessage(EMCInterpreter *interp, EMCState *state) {
  uint16_t type = EMCStateStackVal(state, 0);
  uint16_t stringId = EMCStateStackVal(state, 1);
  uint16_t soundID = EMCStateStackVal(state, 2);
  interp->callbacks.EMCInterpreterCallbacks_PrintMessage(interp, type, stringId,
                                                         soundID);
  return 1;
}

static uint16_t loadBlockProperties(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  interp->callbacks.EMCInterpreterCallbacks_LoadCMZ(interp, file);
  return 1;
}

static uint16_t loadMonsterShapes(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  int16_t monsterId = EMCStateStackVal(state, 1);
  int16_t p2 = EMCStateStackVal(state, 2);
  interp->callbacks.EMCInterpreterCallbacks_LoadMonsterShapes(interp, file,
                                                              monsterId, p2);
  return 1;
}

static uint16_t setSequenceButtons(EMCInterpreter *interp, EMCState *state) {
  int16_t x = EMCStateStackVal(state, 0);
  int16_t y = EMCStateStackVal(state, 1);
  int16_t w = EMCStateStackVal(state, 2);
  int16_t h = EMCStateStackVal(state, 3);
  int16_t enableFlags = EMCStateStackVal(state, 4);
  printf("setSequenceButtons x=%i y=%i w=%i h=%i enableFlags=%i\n", x, y, w, h,
         enableFlags);
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t setSpecialSceneButtons(EMCInterpreter *interp,
                                       EMCState *state) {
  int16_t x = EMCStateStackVal(state, 0);
  int16_t y = EMCStateStackVal(state, 1);
  int16_t w = EMCStateStackVal(state, 2);
  int16_t h = EMCStateStackVal(state, 3);
  int16_t enableFlags = EMCStateStackVal(state, 4);
  printf("setSpecialSceneButtons x=%i y=%i w=%i h=%i enableFlags=%i\n", x, y, w,
         h, enableFlags);
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t loadLevelGraphics(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  uint16_t specialColor = EMCStateStackVal(state, 1);
  uint16_t weight = EMCStateStackVal(state, 2);
  uint16_t vcnLen = EMCStateStackVal(state, 3);
  uint16_t vmpLen = EMCStateStackVal(state, 4);
  uint16_t p5 = EMCStateStackVal(state, 5);
  const char *paletteFile = NULL;
  if (p5 != 0XFFFF) {
    paletteFile = EMCStateGetDataString(state, p5);
  }

  interp->callbacks.EMCInterpreterCallbacks_LoadLevelGraphics(interp, file);
  return 1;
}

static uint16_t setPaletteBrightness(EMCInterpreter *interp, EMCState *state) {
  uint16_t brightness = EMCStateStackVal(state, 0);
  uint16_t otherParam = EMCStateStackVal(state, 1);
  printf("[UNIMPLEMENTED] setPaletteBrightness %X %X\n", brightness,
         otherParam);
  return 1;
}

static uint16_t loadMonsterProperties(EMCInterpreter *interp, EMCState *state) {
  uint16_t monsterIndex = EMCStateStackVal(state, 0);
  uint16_t shapeIndex = EMCStateStackVal(state, 1);
  uint16_t hitChance = EMCStateStackVal(state, 2);
  uint16_t protection = EMCStateStackVal(state, 3);
  uint16_t evadeChance = EMCStateStackVal(state, 4);
  uint16_t speed = EMCStateStackVal(state, 5);
  uint16_t p6 = EMCStateStackVal(state, 6);
  uint16_t p7 = EMCStateStackVal(state, 7);
  uint16_t p8 = EMCStateStackVal(state, 8);
  interp->callbacks.EMCInterpreterCallbacks_LoadMonster(
      interp, monsterIndex, shapeIndex, hitChance, protection, evadeChance,
      speed, p6, p7, p8);
  return 1;
}

static uint16_t initAnimStruct(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  uint16_t index = EMCStateStackVal(state, 1);
  uint16_t x = EMCStateStackVal(state, 2);
  uint16_t y = EMCStateStackVal(state, 3);
  uint16_t offscreenBuffer = EMCStateStackVal(state, 4);
  uint16_t wsaFlags = EMCStateStackVal(state, 5);
  interp->callbacks.EMCInterpreterCallbacks_WSAInit(interp, index, file, x, y,
                                                    offscreenBuffer, wsaFlags);

  return 1;
}

static uint16_t disableControls(EMCInterpreter *interp, EMCState *state) {
  uint16_t mode = EMCStateStackVal(state, 0);
  interp->callbacks.EMCInterpreterCallbacks_DisableControls(interp, mode);
  return 1;
}

static uint16_t enableControls(EMCInterpreter *interp, EMCState *state) {
  interp->callbacks.EMCInterpreterCallbacks_EnableControls(interp);
  return 1;
}

static uint16_t freeAnimStruct(EMCInterpreter *interp, EMCState *state) {
  uint16_t p0 = EMCStateStackVal(state, 0);
  printf("[UNIMPLEMENTED] freeAnimStruct %X\n", p0);
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t loadDoorShapes(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  uint16_t p1 = EMCStateStackVal(state, 1);
  uint16_t p2 = EMCStateStackVal(state, 2);
  uint16_t p3 = EMCStateStackVal(state, 3);
  uint16_t p4 = EMCStateStackVal(state, 4);
  interp->callbacks.EMCInterpreterCallbacks_LoadDoorShapes(interp, file, p1, p2,
                                                           p3, p4);
  return 1;
}

static uint16_t loadLangFile(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  assert(file);
  interp->callbacks.EMCInterpreterCallbacks_LoadLangFile(interp, file);
  return 1;
}

static uint16_t loadLevelShapes(EMCInterpreter *interp, EMCState *state) {
  const char *shpFile =
      EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  const char *datFile =
      EMCStateGetDataString(state, EMCStateStackVal(state, 1));
  interp->callbacks.EMCInterpreterCallbacks_LoadLevelShapes(interp, shpFile,
                                                            datFile);
  return 1;
}

static uint16_t closeLevelShapeFile(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] closeLevelShapeFile\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t assignLevelDecorationShape(EMCInterpreter *interp,
                                           EMCState *state) {
  uint16_t p0 = EMCStateStackVal(state, 0);
  printf("[UNIMPLEMENTED] assignLevelDecorationShape %X\n", p0);
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t resetBlockShapeAssignment(EMCInterpreter *interp,
                                          EMCState *state) {
  printf("[UNIMPLEMENTED] resetBlockShapeAssignment\n");
  // ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t stopPortraitSpeechAnim(EMCInterpreter *interp,
                                       EMCState *state) {
  printf("[UNIMPLEMENTED] stopPortraitSpeechAnim\n");
  return 1;
}

static uint16_t playDialogueTalkText(EMCInterpreter *interp, EMCState *state) {
  int16_t trackId = EMCStateStackVal(state, 0);
  interp->callbacks.EMCInterpreterCallbacks_PlayDialogue(interp, 4, 0, trackId);
  return 1;
}
static uint16_t checkMonsterTypeHostility(EMCInterpreter *interp,
                                          EMCState *state) {
  uint16_t monsterType = EMCStateStackVal(state, 0);
  return interp->callbacks.EMCInterpreterCallbacks_CheckMonsterHostility(
      interp, monsterType);
}

static uint16_t savePage5(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] savePage5\n");
  return 0;
}

static uint16_t restorePage5(EMCInterpreter *interp, EMCState *state) {
  printf("restorePage5\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t moveParty(EMCInterpreter *interp, EMCState *state) {
  printf("moveParty\n");
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t loadNewLevel(EMCInterpreter *interp, EMCState *state) {
  uint16_t level = EMCStateStackVal(state, 0);
  uint16_t startBlock = EMCStateStackVal(state, 1);
  uint16_t startDir = EMCStateStackVal(state, 2);
  interp->callbacks.EMCInterpreterCallbacks_LoadLevel(interp, level, startBlock,
                                                      startDir);
  return 1;
}

static uint16_t checkInventoryFull(EMCInterpreter *interp, EMCState *state) {
  printf("checkInventoryFull\n");
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t createHandItem(EMCInterpreter *interp, EMCState *state) {
  printf("createHandItem\n");
  ASSERT_UNIMPLEMENTED;
  return 1;
}

static uint16_t prepareSpecialScene(EMCInterpreter *interp, EMCState *state) {
  printf("prepareSpecialScene\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t makeItem(EMCInterpreter *interp, EMCState *state) {
  printf("makeItem\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t giveItemToMonster(EMCInterpreter *interp, EMCState *state) {
  printf("giveItemToMonster\n");
  ASSERT_UNIMPLEMENTED;
  return 0;
}

static uint16_t assignCustomSfx(EMCInterpreter *interp, EMCState *state) {
  printf("[UNIMPLEMENTED] assignCustomSfx\n");
  return 1;
}

static uint16_t getGlobalScriptVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t index = EMCStateStackVal(state, 0);
  assert(index < NUM_GLOBAL_SCRIPT_VARS);
  return interp->callbacks.EMCInterpreterCallbacks_GetGlobalScriptVar(interp,
                                                                      index);
}

static uint16_t setGlobalScriptVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t index = EMCStateStackVal(state, 0);
  uint16_t val = EMCStateStackVal(state, 1);
  assert(index < NUM_GLOBAL_SCRIPT_VARS);
  interp->callbacks.EMCInterpreterCallbacks_SetGlobalScriptVar(interp, index,
                                                               val);
  return 1;
}

static uint16_t battleHitSkillTest(EMCInterpreter *interp, EMCState *state) {
  printf("battleHitSkillTest\n");
  return 1;
}

static uint16_t calcInflictableDamagePerItem(EMCInterpreter *interp,
                                             EMCState *state) {
  printf("calcInflictableDamagePerItem\n");
  return 1;
}

static uint16_t getInflictedDamage(EMCInterpreter *interp, EMCState *state) {
  printf("getInflictedDamage\n");
  return 1;
}

static uint16_t inflictDamage(EMCInterpreter *interp, EMCState *state) {
  printf("inflictDamage\n");
  return 1;
}

static uint16_t paletteFlash(EMCInterpreter *interp, EMCState *state) {
  printf("paletteFlash\n");
  return 1;
}

static uint16_t return1(EMCInterpreter *interp, EMCState *state) { return 1; }

static ScriptFunDesc functions[] = {
    {NULL},
    {getWallType, "getWallType"},
    {drawScene, "drawScene"},
    {rollDice, "rollDice"},
    {moveParty, "moveParty"},
    {NULL},
    {NULL},
    {setGameFlag, "setGameFlag"},
    {testGameFlag, "testGameFlag"},
    {loadLevelGraphics, "loadLevelGraphics"},
    // 0X0A
    {loadBlockProperties, "loadBlockProperties"},
    {loadMonsterShapes, "loadMonsterShapes"},
    {NULL},
    {allocItemProperties, "allocItemProperties"},
    {setItemProperty, "setItemProperty"},
    {makeItem, "makeItem"},

    // 0X10
    {NULL},
    {createLevelItem, "createLevelItem"},
    {getItemParam, "getItemParam"},
    {NULL},
    {NULL},
    {loadLevelShapes, "loadLevelShapes"},
    {closeLevelShapeFile, "closeLevelShapeFile"},
    {NULL},
    {loadDoorShapes, "loadDoorShapes"},
    {initAnimStruct, "initAnimStruct"},
    // 0X1A
    {playAnimationPart, "playAnimationPart"},
    {freeAnimStruct, "freeAnimStruct"},
    {getDirection, "getDirection"},
    {NULL},
    {NULL},
    {setSequenceButtons, "setSequenceButtons"},

    // 0X20
    {NULL},
    {checkRectForMousePointer, "checkRectForMousePointer"},
    {clearDialogueField, "clearDialogueField"},
    {setupBackgroundAnimationPart, "setupBackgroundAnimationPart"},
    {startBackgroundAnimation, "startBackgroundAnimation"},
    {hideMouse, "hideMouse"},
    {showMouse, "showMouse"},
    {fadeToBlack, "fadeToBlack"},
    {NULL},
    {loadBitmap, "loadBitmap"},
    // 0X2A
    {stopBackgroundAnimation, "stopBackgroundAnimation"},
    {NULL},
    {NULL},
    {getGlobalScriptVar, "getGlobalScriptVar"},
    {setGlobalScriptVar, "setGlobalScriptVar"},
    {getGlobalVar, "getGlobalVar"},

    // 0X30
    {setGlobalVar, "setGlobalVar"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {assignLevelDecorationShape, "assignLevelDecorationShape"},
    {resetBlockShapeAssignment, "resetBlockShapeAssignment"},
    {copyRegion, "copyRegion"},
    {initMonster, "initMonster"},
    {fadeClearSceneWindow, "fadeClearSceneWindow"},
    // 0X3A
    {fadeSequencePalette, "fadeSequencePalette"},
    {NULL},
    {loadNewLevel, "loadNewLevel"},
    {NULL},
    {NULL},
    {loadMonsterProperties, "loadMonsterProperties"},

    // 0X40
    {battleHitSkillTest, "battleHitSkillTest"},
    {inflictDamage, "inflictDamage"},
    {NULL},
    {NULL},
    {moveMonster, "moveMonster"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {createHandItem, "createHandItem"},
    // 0X4A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {loadTimScript, "loadTimScript"},
    {runTimScript, "runTimScript"},

    // 0X50
    {releaseTimScript, "releaseTimScript"},
    {initSceneWindowDialogue, "initSceneWindowDialogue"},
    {NULL},
    {getItemInHand, "getItemInHand"},
    {NULL},
    {giveItemToMonster, "giveItemToMonster"},
    {loadLangFile, "loadLangFile"},
    {playSoundEffect, "playSoundEffect"},
    {NULL},
    {stopTimScript, "stopTimScript"},
    // 0X5A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {playCharacterScriptChat, "playCharacterScriptChat"},
    {NULL},

    // 0X60
    {NULL},
    {NULL},
    {drawExitButton, "drawExitButton"},
    {loadSoundFile, "loadSoundFile"},
    {playMusicTrack, "playMusicTrack"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X6A
    {stopPortraitSpeechAnim, "stopPortraitSpeechAnim"},
    {setPaletteBrightness, "setPaletteBrightness"},
    {NULL},
    {getInflictedDamage, "getInflictedDamage"},
    {NULL},
    {printMessage, "printMessage"},

    // 0X70
    {NULL},
    {calcInflictableDamagePerItem, "calcInflictableDamagePerItem"},
    {NULL},
    {NULL},
    {checkInventoryFull, "checkInventoryFull"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X7A
    {playDialogueTalkText, "playDialogueTalkText"},
    {checkMonsterTypeHostility, "checkMonsterTypeHostility"},
    {setNextFunc, "setNextFunc"},
    {return1, "stub7D"},
    {NULL},
    {NULL},

    // 0X80
    {NULL},
    {triggerEventOnMouseButtonClick, "triggerEventOnMouseButtonClick"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X8A
    {savePage5, "savePage5"},
    {restorePage5, "restorePage5"},
    {initDialogueSequence, "initDialogueSequence"},
    {restoreAfterDialogueSequence, "restoreAfterDialogueSequence"},
    {setSpecialSceneButtons, "setSpecialSceneButtons"},
    {NULL},

    // 0X90
    {NULL},
    {NULL},
    {prepareSpecialScene, "prepareSpecialScene"},
    {NULL},
    {assignCustomSfx, "assignCustomSfx"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X9A
    {resetPortraitsAndDisableSysTimer, "resetPortraitsAndDisableSysTimer"},
    {enableSysTimer, "enableSysTimer"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // A0
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // B0
    {paletteFlash, "paletteFlash"},
    {NULL},
    {NULL},
    {disableControls, "disableControls"},
    {enableControls, "enableControls"},
};

static const size_t numFunctions = sizeof(functions) / sizeof(ScriptFunDesc);

ScriptFunDesc *getBuiltinFunctions(void) { return functions; }

size_t getNumBuiltinFunctions(void) { return numFunctions; }

void EMCInterpreterExecFunction(EMCInterpreter *interp, EMCState *state,
                                uint8_t funcNum) {

  if (funcNum >= numFunctions) {
    printf("unimplemented func %X\n", funcNum);
    assert(0);
  }
  assert(funcNum < numFunctions);
  ScriptFunDesc desc = functions[funcNum];
  if (desc.fun == NULL) {
    printf("unimplemented func %X\n", funcNum);
    assert(0);
  }
  state->retValue = desc.fun(interp, state);
}
