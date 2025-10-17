#pragma once
#include "SDL_render.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_fnt.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "game_tim_animator.h"
#include "geometry.h"
#include "pak_file.h"
#include "script.h"
#include <stdint.h>

#define SCREEN_FACTOR 4

#define PIX_BUF_WIDTH 320
#define PIX_BUF_HEIGHT 200

#define DIALOG_BUFFER_SIZE (size_t)1024

#define MAX_MONSTER_PROPERTIES 5
#define MAX_MONSTERS 30
#define MAX_IN_GAME_ITEMS 400

#define NUM_GLOBAL_SCRIPT_VARS 24

typedef struct {
  uint8_t level;
  uint16_t itemPropertyIndex;

} Item;

typedef struct {
  uint16_t stringId;
  uint16_t shapeId;
  uint16_t type;
  uint16_t scriptFun;
  uint16_t might;
  uint16_t skill;
  uint16_t protection;
  uint16_t flags;
} ItemProperty;

typedef struct {
  uint8_t shapeIndex;
  uint8_t maxWidth;
  uint16_t fightingStats[9];
  uint16_t itemsMight[8];
  uint16_t protectionAgainstItems[8];
  uint16_t itemProtection;
  uint16_t hitPoints;
  uint8_t speedTotalWaitTicks;
  uint8_t skillLevel;
  uint16_t flags;
  uint16_t _unknown;
  uint16_t numDistAttacks;
  uint16_t numDistWeapons;
  uint16_t distWeapons[3];
  uint8_t attackSkillChance;
  uint8_t attackSkillType;
  uint8_t defenseSkillChance;
  uint8_t defenseSkillType;
  uint8_t sounds[3];
} MonsterProperties;

typedef struct {
  uint8_t destDirection;
  int8_t shiftStep;
  uint16_t destX;
  uint16_t destY;

  int8_t hitOffsX;
  int8_t hitOffsY;
  uint8_t currentSubFrame;
  uint8_t mode;
  int8_t fightCurTick;
  uint8_t id;
  uint8_t direction;
  uint8_t facing;
  uint16_t flags;
  uint16_t damageReceived;
  int16_t hitPoints;
  uint8_t speedTick;
  uint8_t type;
  MonsterProperties *properties;
  uint8_t numDistAttacks;
  uint8_t curDistWeapon;
  int8_t distAttackTick;
  uint16_t assignedItems;
  uint8_t equipmentShapes[4];
} Monster;

typedef struct {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  MazeHandle mazHandle;
  WllHandle wllHandle;
  DatHandle datHandle;
  SHPHandle shpHandle;
  LangHandle levelLang;

  SHPHandle doors;
  SHPHandle monsterShapes[MAX_MONSTERS];

  MonsterProperties monsterProperties[MAX_MONSTER_PROPERTIES];
  Monster monsters[MAX_MONSTERS];

} LevelContext;

typedef enum {
  GameState_Invalid = 0,
  GameState_PlayGame = 1,
  GameState_TimAnimation = 2,
  GameState_ShowInventory = 3,
  GameState_GrowDialogBox = 4,
  GameState_ShrinkDialogBox = 5,
} GameState;

typedef enum {
  DialogState_None,
  DialogState_InProgress,
  DialogState_Done,
} DialogState;

typedef struct {
  Point pos;
  uint8_t pending;
  uint8_t isRightClick;
} MouseEvent;

typedef struct _GameContext {
  GameState state;
  GameState prevState;

  int fadeOutFrames;
  int dialogBoxFrames;
  int showBigDialog;

  int drawExitSceneButton;
  int exitSceneButtonDisabled;

  char *buttonText[3];

  MouseEvent mouseEv;
  uint16_t currentBock;
  Orientation orientation;

  LevelContext *level;
  int levelId;

  SDL_Texture *backgroundPixBuf;
  SDL_Texture *foregroundPixBuf;
  SDL_Renderer *renderer;
  SDL_Window *window;
  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];

  CPSImage playField;
  SHPHandle itemShapes;
  LangHandle lang;

  SHPHandle charFaces[NUM_CHARACTERS];
  SAVCharacter chars[NUM_CHARACTERS];
  uint8_t selectedChar;

  CPSImage inventoryBackground;

  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  PAKFile generalPak;

  INFScript script;
  INFScript iniScript;
  INFScript itemScript;

  EMCInterpreter interp;
  EMCState interpState;

  char *dialogTextBuffer;
  char *dialogText; // will either be NULL or pointing to dialogTextBuffer

  uint16_t globalScriptVars[NUM_GLOBAL_SCRIPT_VARS];

  FNTHandle defaultFont;
  uint8_t gameFlags[100];

  GameTimAnimator timAnimator;

  uint16_t inventory[INVENTORY_SIZE];
  uint16_t inventoryIndex;
  uint16_t itemInHand;

  uint8_t *defaultPalette;

  int controlDisabled;

  SDL_Cursor *cursor;

  ItemProperty *itemProperties;
  Item *itemsInGame;
  uint16_t itemsCount;

  CPSImage loadedbitMap;
  int _shouldRun;

  DialogState dialogState;

  int _noClip;
} GameContext;

void GameContextRelease(GameContext *gameCtx);
int GameContextInit(GameContext *gameCtx);
int GameContextStartup(GameContext *ctx);

int GameContextLoadLevel(GameContext *ctx, int levelNum);
int GameContextRunLevelInitScript(GameContext *gameCtx);
int GameContextRunScript(GameContext *gameCtx, int function);

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag);
void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag, uint16_t val);
void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag);

void GameContextSetState(GameContext *gameCtx, GameState newState);

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId);
uint8_t GameContextGetNumChars(const GameContext *ctx);

uint16_t GameContextGetString(const GameContext *ctx, uint16_t stringId,
                              char *outBuffer, size_t outBufferSize);

uint16_t GameContextGetItemSHPFrameIndex(GameContext *gameCtx, uint16_t itemId);

void GameContextInitSceneDialog(GameContext *gameCtx);
void GameContextCleanupSceneDialog(GameContext *gameCtx);
