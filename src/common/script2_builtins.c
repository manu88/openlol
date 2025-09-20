#include "script2_builtins.h"
#include "bytes.h"
#include "script2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t (*ScriptFunction)(EMCInterpreter *interp, EMCState *state);

typedef struct {
  ScriptFunction fun;
  char name[32];
} ScriptFunDesc;

static uint16_t getWallType(EMCInterpreter *interp, EMCState *state) {
  printf("getWallType\n");
  return 0;
}
static uint16_t drawScene(EMCInterpreter *interp, EMCState *state) {
  printf("drawScene\n");
  return 0;
}
static uint16_t rollDice(EMCInterpreter *interp, EMCState *state) {
  printf("rollDice\n");
  return 0;
}
static uint16_t enableSysTimer(EMCInterpreter *interp, EMCState *state) {
  printf("enableSysTimer\n");
  return 0;
}
static uint16_t setGameFlag(EMCInterpreter *interp, EMCState *state) {
  printf("setGameFlag\n");
  return 0;
}

static uint8_t _flagsTable[100];

static uint16_t queryGameFlag(uint16_t flag) {
  return ((_flagsTable[flag >> 3] >> (flag & 7)) & 1);
}
static uint16_t testGameFlag(EMCInterpreter *interp, EMCState *state) {
  uint16_t p = EMCStateStackVal(state, 0);
  if (p < 0) {
    return 0;
  }

  printf("testGameFlag %X\n", p);
  assert((p >> 3) >= 0 && (p >> 3) <= 100);
  return queryGameFlag(p);
}

static uint16_t getItemParam(EMCInterpreter *interp, EMCState *state) {
  printf("getItemParam\n");
  return 0;
}

static uint16_t playAnimationPart(EMCInterpreter *interp, EMCState *state) {
  printf("playAnimationPart\n");
  return 0;
}

static uint16_t getDirection(EMCInterpreter *interp, EMCState *state) {
  printf("getDirection\n");
  return 1;
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
  printf("loadBitmap\n");
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

static uint16_t setGlobalVar(EMCInterpreter *interp, EMCState *state) {
  uint16_t how = EMCStateStackVal(state, 0);
  uint16_t a = EMCStateStackVal(state, 1);
  uint16_t b = EMCStateStackVal(state, 2);

  printf("setGlobalVar %x %x %x\n", how, a, b);
  assert(how < 0x0d);
  switch (how) {
  case 0X0: {
    uint16_t x = 0;
    uint16_t y = 0;
    calcCoordinates(&x, &y, b, 0x80, 0x80);
    uint16_t xx = b & 0x1F;
    uint16_t yx = b >> 5;
    printf("x=%x y=%x\n", xx, yx);
  } break;
  case 0X0A:
    break;
  default:
    printf("setGlobalVar: unimplemented %x %x %x\n", how, a, b);
    break;
  }
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
  printf("loadTimScript\n");
  return 0;
}

static uint16_t runTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t p0 = EMCStateStackVal(state, 0);
  int16_t p1 = EMCStateStackVal(state, 0);
  printf("runTimScript %X %X\n", p0, p1);
  return 0;
}
static uint16_t releaseTimScript(EMCInterpreter *interp, EMCState *state) {
  printf("releaseTimScript\n");
  return 0;
}
static uint16_t getItemInHand(EMCInterpreter *interp, EMCState *state) {
  printf("getItemInHand\n");
  return 0;
}
static uint16_t playSoundEffect(EMCInterpreter *interp, EMCState *state) {
  printf("playSoundEffect\n");
  return 0;
}
static uint16_t stopTimScript(EMCInterpreter *interp, EMCState *state) {
  int16_t scriptId = EMCStateStackVal(state, 0);
  printf("stopTimScript scriptId=%X\n", scriptId);
  return 0;
}

static void getLangString(uint16_t id) {
  if (id == 0xFFFF)
    return;

  uint16_t realId = id & 0x3FFF;
  int useLevelFile = 0;
  if (id & 0x4000) {
    useLevelFile = 0;
  } else {
    useLevelFile = 1;
  }
  printf("real lang string id=%i uselevel=%i\n", realId, useLevelFile);
}

static uint16_t playCharacterScriptChat(EMCInterpreter *interp,
                                        EMCState *state) {
  int16_t charId = EMCStateStackVal(state, 0);
  int16_t mode = EMCStateStackVal(state, 1);
  int16_t stringId = EMCStateStackVal(state, 2);
  printf("playCharacterScriptChat charId=%i mode=%i stringId=%i\n", charId,
         mode, stringId);
  getLangString(stringId);
  return 0;
}
static uint16_t drawExitButton(EMCInterpreter *interp, EMCState *state) {
  printf("drawExitButton\n");
  return 0;
}

static uint16_t printMessage(EMCInterpreter *interp, EMCState *state) {
  int16_t type = EMCStateStackVal(state, 0);
  int16_t stringID = EMCStateStackVal(state, 1);
  int16_t soundID = EMCStateStackVal(state, 2);

  printf("printMessage type=0X%X stringID=0X%X soundID=0X%X\n", type, stringID,
         soundID);
  getLangString(stringID);
  return 0;
}
static uint16_t playDialogueTalkText(EMCInterpreter *interp, EMCState *state) {
  int16_t track = EMCStateStackVal(state, 0);
  printf("playDialogueTalkText track=%x\n", track);
  return 0;
}
static uint16_t checkMonsterTypeHostility(EMCInterpreter *interp,
                                          EMCState *state) {
  int16_t monsterType = EMCStateStackVal(state, 0);
  printf("checkMonsterTypeHostility monsterType=%x\n", monsterType);
  return 0;
}
static uint16_t savePage5(EMCInterpreter *interp, EMCState *state) {
  printf("savePage5\n");
  return 0;
}
static uint16_t prepareSpecialScene(EMCInterpreter *interp, EMCState *state) {
  printf("prepareSpecialScene\n");
  return 0;
}

static uint16_t getGlobalScriptVar(EMCInterpreter *interp, EMCState *state) {
  printf("getGlobalScriptVar\n");
  return 0;
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
    {NULL},
    {NULL},
    {NULL},
    {setGameFlag, "setGameFlag"},
    {testGameFlag, "testGameFlag"},
    {NULL},
    // 0X0A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X10
    {NULL},
    {NULL},
    {getItemParam, "getItemParam"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X1A
    {playAnimationPart, "playAnimationPart"},
    {NULL},
    {getDirection, "getDirection"},
    {NULL},

    {NULL},
    {NULL},

    // 0X20
    {NULL},
    {checkRectForMousePointer, "checkRectForMousePointer"},
    {clearDialogueField, "clearDialogueField"},
    {setupBackgroundAnimationPart, "setupBackgroundAnimationPart"},
    {NULL},
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
    {NULL},

    // 0X30
    {setGlobalVar, "setGlobalVar"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {copyRegion, "copyRegion"},
    {NULL},
    {NULL},
    // 0X3A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

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
    {NULL},
    // 0X4A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {loadTimScript, "loadTimScript"},
    {runTimScript, "runTimScript"},

    // 0X50
    {releaseTimScript, "releaseTimScript"},
    {NULL},
    {NULL},
    {getItemInHand, "getItemInHand"},
    {NULL},
    {NULL},
    {NULL},
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
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X6A
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {printMessage, "printMessage"},

    // 0X70
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
    // 0X7A
    {playDialogueTalkText, "playDialogueTalkText"},
    {checkMonsterTypeHostility, "checkMonsterTypeHostility"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X80
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
    // 0X8A
    {savePage5, "savePage5"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    // 0X90
    {NULL},
    {NULL},
    {prepareSpecialScene, "prepareSpecialScene"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    // 0X9B
    {enableSysTimer, "enableSysTimer"},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},

    //
};

static const size_t numFunctions = sizeof(functions) / sizeof(ScriptFunDesc);

void EMCInterpreterExecFunction(EMCInterpreter *interp, EMCState *state,
                                uint8_t funcNum) {

  if (funcNum >= numFunctions) {
    printf("unimplemented func %X\n", interp->_parameter);
  }
  assert(funcNum < numFunctions);
  ScriptFunDesc desc = functions[funcNum];
  if (desc.fun == NULL) {
    printf("unimplemented func %X\n", interp->_parameter);
  }
  state->retValue = desc.fun(interp, state);
}
