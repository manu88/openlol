#pragma once
#include "SDL_render.h"
#include "animator.h"
#include "audio.h"
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
#include "menu.h"
#include "pak_file.h"
#include "script.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_FACTOR 4

#define PIX_BUF_WIDTH 320
#define PIX_BUF_HEIGHT 200

#define DIALOG_BUFFER_SIZE (size_t)1024

#define MAX_MONSTER_PROPERTIES 5
#define MAX_MONSTERS 30
#define MAX_IN_GAME_ITEMS 400

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
  GameState_Invalid,
  GameState_MainMenu,
  GameState_PlayGame,
  GameState_GameMenu,
  GameState_TimAnimation,
  GameState_ShowInventory,
  GameState_GrowDialogBox,
  GameState_ShrinkDialogBox,

  GameState_ShowMap,
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

#define NUM_GAME_FLAGS 100

#define INVENTORY_TYPES_NUM 7
static const uint8_t inventoryTypeForId[] = {0, 1, 2, 6, 3, 1, 1, 3, 5, 4};

typedef struct _GameContext {
  AudioSystem audio;
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

  SDL_Texture *pixBuf;
  SDL_Renderer *renderer;
  SDL_Window *window;
  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];

  SHPHandle itemShapes;
  Language language;
  LangHandle lang;

  SHPHandle charFaces[NUM_CHARACTERS];
  SAVCharacter chars[NUM_CHARACTERS];
  uint8_t selectedChar;
  uint8_t selectedCharIsCastingSpell;

  SHPHandle automapShapes;
  SHPHandle gameShapes;

  CPSImage inventoryBackgrounds[INVENTORY_TYPES_NUM];
  CPSImage playField;
  CPSImage loadedbitMap;
  CPSImage mapBackground;

  CPSImage gameTitle;

  SDL_Surface *textSurface;
  SDL_Texture *textTexture;

  INFScript script;
  INFScript itemScript;

  uint16_t nextFunc;

  EMCInterpreter interp;
  EMCState interpState;

  char *dialogTextBuffer;
  char *dialogText; // will either be NULL or pointing to dialogTextBuffer

  uint16_t globalScriptVars[NUM_GLOBAL_SCRIPT_VARS];

  FNTHandle defaultFont;
  FNTHandle font6p;
  uint8_t gameFlags[NUM_GAME_FLAGS];

  GameTimInterpreter timInterpreter;
  Animator animator;

  uint16_t inventory[INVENTORY_SIZE];
  uint16_t inventoryIndex;
  uint16_t itemIndexInHand;
  uint16_t credits;

  uint8_t *defaultPalette;

  int controlDisabled;

  SDL_Cursor *cursor;

  ItemProperty *itemProperties;
  GameObject *itemsInGame;
  uint16_t itemsCount;

  int _shouldRun;

  DialogState dialogState;

  int _noClip;

  Menu *currentMenu;
  int shouldUpdate;

  char *savDir;

  PAKFile currentTlkFile;
  int currentTlkFileIndex;
  PAKFile sfxPak;

  ConfigHandle conf;
} GameContext;

void GameContextRelease(GameContext *gameCtx);
int GameContextInit(GameContext *gameCtx, Language lang);
int GameContextStartup(GameContext *ctx);

int GameContextSetSavDir(GameContext *gameCtx, const char *path);

typedef struct _SAVFile {
  char *fullpath;
  char *savName;
} SAVFile;

void SAVFilesRelease(SAVFile *files, size_t numFiles);

SAVFile *GameContextListSavFiles(GameContext *gameCtx, size_t *numSavFiles);

int GameContextNewGame(GameContext *gameCtx);

int GameContextLoadLevel(GameContext *ctx, int levelNum);
int GameContextLoadChars(GameContext *ctx);
int GameContextRunLevelInitScript(GameContext *gameCtx);
int GameContextRunScript(GameContext *gameCtx, int function);

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag);
void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag);
void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag);

void GameContextSetState(GameContext *gameCtx, GameState newState);

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId);
uint8_t GameContextGetNumChars(const GameContext *ctx);

uint16_t GameContextGetString(const GameContext *ctx, uint16_t stringId,
                              char *outBuffer, size_t outBufferSize);

// string needs to be freed!
char *GameContextGetString2(const GameContext *ctx, uint16_t stringId);
uint16_t GameContextGetLevelName(const GameContext *gameCtx, char *outBuffer,
                                 size_t outBufferSize);

uint16_t GameContextGetItemSHPFrameIndex(GameContext *gameCtx, uint16_t itemId);

void GameContextInitSceneDialog(GameContext *gameCtx);
void GameContextCleanupSceneDialog(GameContext *gameCtx);

uint16_t GameContextCreateItem(GameContext *gameCtx, uint16_t itemType);
void GameContextDeleteItem(GameContext *gameCtx, uint16_t itemIndex);

void GameContextUpdateCursor(GameContext *gameCtx);
void GameContextExitGame(GameContext *gameCtx);
int GameContextLoadSaveFile(GameContext *gameCtx, const char *filepath);

void GameContextLoadBackgroundInventoryIfNeeded(GameContext *gameCtx,
                                                int charId);

void GameContextLoadTLKFile(GameContext *gameCtx, int levelIndex);
void GameContextPlayDialogSpeech(GameContext *gameCtx, int16_t charId,
                                 uint16_t strId);
