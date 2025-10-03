#include "script_builtins.h"
#include "script.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t getWallType(EMCInterpreter *interp, EMCState *state) {
  printf("getWallType\n");
  return 1;
}
static uint16_t drawScene(EMCInterpreter *interp, EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  printf("drawScene %X\n", p0);
  return 1;
}
static uint16_t rollDice(EMCInterpreter *interp, EMCState *state) {
  int16_t times = EMCStateStackVal(state, 0);
  int16_t max = EMCStateStackVal(state, 1);
  printf("rollDice times=%i max=%i\n", times, max);
  return 1;
}

static uint16_t enableSysTimer(EMCInterpreter *interp, EMCState *state) {
  printf("enableSysTimer\n");
  return 0;
}

static uint16_t initDialogueSequence(EMCInterpreter *interp, EMCState *state) {
  printf("initDialogueSequence\n");
  return 0;
}

static uint16_t restoreAfterDialogueSequence(EMCInterpreter *interp,
                                             EMCState *state) {
  printf("restoreAfterDialogueSequence\n");
  return 0;
}

static uint16_t resetPortraitsAndDisableSysTimer(EMCInterpreter *interp,
                                                 EMCState *state) {
  printf("enableSysTimer\n");
  return 0;
}

static uint16_t setGameFlag(EMCInterpreter *interp, EMCState *state) {
  uint16_t flag = EMCStateStackVal(state, 0);
  uint16_t val = EMCStateStackVal(state, 1);
  if (interp->callbacks.EMCInterpreterCallbacks_SetGameFlag) {
    interp->callbacks.EMCInterpreterCallbacks_SetGameFlag(interp, flag, val);
  }
  return 1;
}

static uint16_t testGameFlag(EMCInterpreter *interp, EMCState *state) {
  uint16_t p = EMCStateStackVal(state, 0);
  if (interp->callbacks.EMCInterpreterCallbacks_TestGameFlag(interp, p)) {
    return interp->callbacks.EMCInterpreterCallbacks_TestGameFlag(interp, p);
  }
  return 0;
}

static uint16_t getItemParam(EMCInterpreter *interp, EMCState *state) {
  printf("getItemParam\n");
  return 0;
}

static uint16_t createLevelItem(EMCInterpreter *interp, EMCState *state) {
  printf("createLevelItem\n");
  return 0;
}

static uint16_t playAnimationPart(EMCInterpreter *interp, EMCState *state) {
  printf("playAnimationPart\n");
  return 0;
}

static uint16_t setNextFunc(EMCInterpreter *interp, EMCState *state) {
  uint16_t p = EMCStateStackVal(state, 0);
  printf("setNextFunc %x\n", p);
  return 0;
}

static uint16_t getDirection(EMCInterpreter *interp, EMCState *state) {
  if (interp->callbacks.EMCInterpreterCallbacks_GetDirection) {
    return interp->callbacks.EMCInterpreterCallbacks_GetDirection(interp);
  }
  return 0;
}

static uint16_t checkRectForMousePointer(EMCInterpreter *interp,
                                         EMCState *state) {
  printf("checkRectForMousePointer\n");
  return 0;
}
static uint16_t clearDialogueField(EMCInterpreter *interp, EMCState *state) {
  printf("clearDialogueField\n");
  return 0;
}
static uint16_t setupBackgroundAnimationPart(EMCInterpreter *interp,
                                             EMCState *state) {
  printf("setupBackgroundAnimationPart\n");
  return 0;
}
static uint16_t hideMouse(EMCInterpreter *interp, EMCState *state) {
  printf("hideMouse\n");
  return 0;
}
static uint16_t showMouse(EMCInterpreter *interp, EMCState *state) {
  printf("showMouse\n");
  return 0;
}
static uint16_t fadeToBlack(EMCInterpreter *interp, EMCState *state) {
  printf("fadeToBlack\n");
  return 0;
}
static uint16_t loadBitmap(EMCInterpreter *interp, EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  int16_t p1 = EMCStateStackVal(state, 1);
  const char *f = EMCStateGetDataString(state, p0);
  printf("loadBitmap %X %X '%s'\n", p0, p1, f);
  return 0;
}
static uint16_t stopBackgroundAnimation(EMCInterpreter *interp,
                                        EMCState *state) {
  printf("stopBackgroundAnimation\n");
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
  if (interp->callbacks.EMCInterpreterCallbacks_GetGlobalVar) {
    return interp->callbacks.EMCInterpreterCallbacks_GetGlobalVar(interp, how,
                                                                  a);
  }
  return 0;
}

static uint16_t initMonster(EMCInterpreter *interp, EMCState *state) {
  printf("initMonster\n");
  return 1;
}

static uint16_t setGlobalVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t how = EMCStateStackVal(state, 0);
  uint16_t a = EMCStateStackVal(state, 1);
  uint16_t b = EMCStateStackVal(state, 2);
  if (interp->callbacks.EMCInterpreterCallbacks_SetGlobalVar) {
    return interp->callbacks.EMCInterpreterCallbacks_SetGlobalVar(interp, how,
                                                                  a, b);
  }
  return 0;
}

static uint16_t loadSoundFile(EMCInterpreter *interp, EMCState *state) {
  printf("loadSoundFile\n");
  return 0;
}

static uint16_t playMusicTrack(EMCInterpreter *interp, EMCState *state) {
  printf("playMusicTrack\n");
  return 0;
}

static uint16_t fadeClearSceneWindow(EMCInterpreter *interp, EMCState *state) {
  printf("fadeClearSceneWindow\n");
  return 0;
}

static uint16_t fadeSequencePalette(EMCInterpreter *interp, EMCState *state) {
  printf("fadeSequencePalette\n");
  return 0;
}

static uint16_t initSceneWindowDialogue(EMCInterpreter *interp,
                                        EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  printf("initSceneWindowDialogue %x\n", p0);
  return 0;
}

static uint16_t startBackgroundAnimation(EMCInterpreter *interp,
                                         EMCState *state) {
  printf("startBackgroundAnimation\n");
  return 0;
}

static uint16_t copyRegion(EMCInterpreter *interp, EMCState *state) {
  printf("copyRegion\n");
  return 0;
}
static uint16_t moveMonster(EMCInterpreter *interp, EMCState *state) {
  printf("moveMonster\n");
  return 0;
}

static uint16_t loadTimScript(EMCInterpreter *interp, EMCState *state) {
  uint16_t scriptId = EMCStateStackVal(state, 0);
  uint16_t stringId = EMCStateStackVal(state, 1);
  const char *str = EMCStateGetDataString(state, stringId);
  if (interp->callbacks.EMCInterpreterCallbacks_LoadTimScript) {
    interp->callbacks.EMCInterpreterCallbacks_LoadTimScript(interp, scriptId,
                                                            str);
  }
  return 1;
}

static uint16_t runTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  int16_t loop = EMCStateStackVal(state, 1);
  if (interp->callbacks.EMCInterpreterCallbacks_RunTimScript) {
    interp->callbacks.EMCInterpreterCallbacks_RunTimScript(interp, scriptId,
                                                           loop);
  }
  return 1;
}

static uint16_t stopTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  printf("stopTimScript scriptId=%X\n", scriptId);
  return 0;
}

static uint16_t releaseTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  if (interp->callbacks.EMCInterpreterCallbacks_ReleaseTimScript) {
    interp->callbacks.EMCInterpreterCallbacks_ReleaseTimScript(interp,
                                                               scriptId);
  }
  return 1;
}
static uint16_t getItemInHand(EMCInterpreter *interp, EMCState *state) {
  printf("getItemInHand\n");
  return 0;
}
static uint16_t playSoundEffect(EMCInterpreter *interp, EMCState *state) {
  printf("playSoundEffect\n");
  return 0;
}

static uint16_t playCharacterScriptChat(EMCInterpreter *interp,
                                        EMCState *state) {
  int16_t charId = EMCStateStackVal(state, 0);
  int16_t mode = EMCStateStackVal(state, 1);
  int16_t stringId = EMCStateStackVal(state, 2);

  if (interp->callbacks.EMCInterpreterCallbacks_PlayDialogue) {
    interp->callbacks.EMCInterpreterCallbacks_PlayDialogue(interp, charId, mode,
                                                           stringId);
  }
  return 1;
}

static uint16_t drawExitButton(EMCInterpreter *interp, EMCState *state) {
  printf("drawExitButton\n");
  return 0;
}

static uint16_t triggerEventOnMouseButtonClick(EMCInterpreter *interp,
                                               EMCState *state) {
  int evt = EMCStateStackVal(state, 0);
  printf("triggerEventOnMouseButtonClick evt=%X\n", evt);
  return 1;
}

static uint16_t printMessage(EMCInterpreter *interp, EMCState *state) {
  uint16_t type = EMCStateStackVal(state, 0);
  uint16_t stringId = EMCStateStackVal(state, 1);
  uint16_t soundID = EMCStateStackVal(state, 2);

  if (interp->callbacks.EMCInterpreterCallbacks_PrintMessage) {
    interp->callbacks.EMCInterpreterCallbacks_PrintMessage(interp, type,
                                                           stringId, soundID);
  }
  return 1;
}

static uint16_t loadBlockProperties(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  if (interp->callbacks.EMCInterpreterCallbacks_LoadCMZ) {
    interp->callbacks.EMCInterpreterCallbacks_LoadCMZ(interp, file);
  }
  return 1;
}

static uint16_t loadMonsterShapes(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  int16_t p1 = EMCStateStackVal(state, 1);
  int16_t p2 = EMCStateStackVal(state, 2);
  printf("loadMonsterShapes '%s' %X %X\n", file, p1, p2);
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

  printf("loadLevelGraphics '%s' %X %X %X %X %X %s\n", file, specialColor,
         weight, vcnLen, vmpLen, p5, paletteFile);
  // files: VCF VCN VMP
  if (interp->callbacks.EMCInterpreterCallbacks_LoadLevelGraphics) {
    interp->callbacks.EMCInterpreterCallbacks_LoadLevelGraphics(interp, file);
  }
#if 0

#endif
  return 1;
}

static uint16_t setPaletteBrightness(EMCInterpreter *interp, EMCState *state) {
  printf("setPaletteBrightness\n");
  return 1;
}

static uint16_t loadMonsterProperties(EMCInterpreter *interp, EMCState *state) {

  uint16_t monsterIndex = EMCStateStackVal(state, 0);
  uint16_t shapeIndex = EMCStateStackVal(state, 1);

  uint16_t fightingStats[9] = {0};
  fightingStats[0] = (EMCStateStackVal(state, 2) << 8) / 100; // hit chance
  fightingStats[1] = 256;
  fightingStats[2] = (EMCStateStackVal(state, 3) << 8) / 100; // protection
  fightingStats[3] = EMCStateStackVal(state, 4);              // evade chance
  fightingStats[4] = (EMCStateStackVal(state, 5) << 8) / 100; // speed
  fightingStats[5] = (EMCStateStackVal(state, 6) << 8) / 100;
  fightingStats[6] = (EMCStateStackVal(state, 7) << 8) / 100;
  fightingStats[7] = (EMCStateStackVal(state, 8) << 8) / 100;
  fightingStats[8] = 0;
  printf("loadMonsterProperties monster %X shape %X hit=%i prot=%i evade=%i "
         "speed=%i\n",
         monsterIndex, shapeIndex, fightingStats[0], fightingStats[2],
         fightingStats[3], fightingStats[4]);
  return 1;
}

static uint16_t initAnimStruct(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  uint16_t p1 = EMCStateStackVal(state, 1);
  uint16_t p2 = EMCStateStackVal(state, 2);
  uint16_t p3 = EMCStateStackVal(state, 3);
  uint16_t p4 = EMCStateStackVal(state, 4);
  uint16_t p5 = EMCStateStackVal(state, 5);
  printf("initAnimStruct '%s' %X %X %X %X %X\n", file, p1, p2, p3, p4, p5);
  return 1;
}

static uint16_t disableControls(EMCInterpreter *interp, EMCState *state) {
  uint16_t mode = EMCStateStackVal(state, 0);
  printf("disableControls %X\n", mode);
  return 1;
}

static uint16_t enableControls(EMCInterpreter *interp, EMCState *state) {
  printf("enableControls\n");
  return 1;
}

static uint16_t freeAnimStruct(EMCInterpreter *interp, EMCState *state) {
  uint16_t p0 = EMCStateStackVal(state, 0);
  printf("freeAnimStruct %X\n", p0);
  return 1;
}

static uint16_t loadDoorShapes(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  uint16_t p1 = EMCStateStackVal(state, 1);
  uint16_t p2 = EMCStateStackVal(state, 2);
  printf("loadDoorShapes '%s' %X %X\n", file, p1, p2);
  return 1;
}

static uint16_t loadLangFile(EMCInterpreter *interp, EMCState *state) {
  const char *file = EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  assert(file);
  if (interp->callbacks.EMCInterpreterCallbacks_LoadLangFile) {
    interp->callbacks.EMCInterpreterCallbacks_LoadLangFile(interp, file);
  }
  return 1;
}

static uint16_t loadLevelShapes(EMCInterpreter *interp, EMCState *state) {
  const char *shpFile =
      EMCStateGetDataString(state, EMCStateStackVal(state, 0));
  const char *datFile =
      EMCStateGetDataString(state, EMCStateStackVal(state, 1));
  if (interp->callbacks.EMCInterpreterCallbacks_LoadLevelShapes) {
    interp->callbacks.EMCInterpreterCallbacks_LoadLevelShapes(interp, shpFile,
                                                              datFile);
  }
  printf("loadLevelShapes '%s' '%s'\n", shpFile, datFile);
  return 1;
}

static uint16_t closeLevelShapeFile(EMCInterpreter *interp, EMCState *state) {
  printf("closeLevelShapeFile\n");
  return 1;
}

static uint16_t assignLevelDecorationShape(EMCInterpreter *interp,
                                           EMCState *state) {
  uint16_t p0 = EMCStateStackVal(state, 0);
  printf("assignLevelDecorationShape %X\n", p0);
  return 1;
}

static uint16_t resetBlockShapeAssignment(EMCInterpreter *interp,
                                          EMCState *state) {
  printf("resetBlockShapeAssignment\n");
  return 1;
}

static uint16_t playDialogueTalkText(EMCInterpreter *interp, EMCState *state) {
  int16_t track = EMCStateStackVal(state, 0);
  printf("playDialogueTalkText track=%x\n", track);
  return 0;
}
static uint16_t checkMonsterTypeHostility(EMCInterpreter *interp,
                                          EMCState *state) {
  uint16_t monsterType = EMCStateStackVal(state, 0);
  printf("checkMonsterTypeHostility monsterType=%x\n", monsterType);
  return 0;
}

static uint16_t savePage5(EMCInterpreter *interp, EMCState *state) {
  printf("savePage5\n");
  return 0;
}

static uint16_t restorePage5(EMCInterpreter *interp, EMCState *state) {
  printf("restorePage5\n");
  return 0;
}

static uint16_t moveParty(EMCInterpreter *interp, EMCState *state) {
  printf("moveParty\n");
  return 1;
}

static uint16_t loadNewLevel(EMCInterpreter *interp, EMCState *state) {
  uint16_t level = EMCStateStackVal(state, 0);
  uint16_t startBlock = EMCStateStackVal(state, 1);
  uint16_t startDir = EMCStateStackVal(state, 2);
  if (interp->callbacks.EMCInterpreterCallbacks_LoadLevel) {
    interp->callbacks.EMCInterpreterCallbacks_LoadLevel(interp, level,
                                                        startBlock, startDir);
  }
  return 1;
}

static uint16_t checkInventoryFull(EMCInterpreter *interp, EMCState *state) {
  printf("checkInventoryFull\n");
  return 1;
}

static uint16_t createHandItem(EMCInterpreter *interp, EMCState *state) {
  printf("createHandItem\n");
  return 1;
}

static uint16_t prepareSpecialScene(EMCInterpreter *interp, EMCState *state) {
  printf("prepareSpecialScene\n");
  return 0;
}

static uint16_t makeItem(EMCInterpreter *interp, EMCState *state) {
  printf("makeItem\n");
  return 0;
}

static uint16_t giveItemToMonster(EMCInterpreter *interp, EMCState *state) {
  printf("giveItemToMonster\n");
  return 0;
}

static uint16_t assignCustomSfx(EMCInterpreter *interp, EMCState *state) {
  printf("assignCustomSfx\n");
  return 0;
}

static uint16_t getGlobalScriptVar(EMCInterpreter *interp, EMCState *state) {
  printf("getGlobalScriptVar\n");
  return 1;
}

static uint16_t setGlobalScriptVar(EMCInterpreter *interp, EMCState *state) {
  printf("setGlobalScriptVar\n");
  return 0;
}

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
    {NULL},
    {NULL},
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
    {NULL},
    {NULL},
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
    {NULL},
    {setPaletteBrightness, "setPaletteBrightness"},
    {NULL},
    {NULL},
    {NULL},
    {printMessage, "printMessage"},

    // 0X70
    {NULL},
    {NULL},
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
    {NULL},
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
    {NULL},
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
