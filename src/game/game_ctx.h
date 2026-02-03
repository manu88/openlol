#pragma once

#include "animator.h"
#include "audio.h"
#include "config.h"
#include "display.h"
#include "engine.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "game_tim_animator.h"
#include "geometry.h"
#include "level.h"
#include "menu.h"
#include "monster.h"
#include "pak_file.h"
#include "script.h"
#include "spells.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_FACTOR 4

#define PIX_BUF_WIDTH 320
#define PIX_BUF_HEIGHT 200

#define DIALOG_BUFFER_SIZE (size_t)1024

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

typedef enum {
  GameState_Invalid,
  GameState_MainMenu,
  GameState_Prologue,
  GameState_PlayGame,
  GameState_GameMenu,
  GameState_TimAnimation,
  GameState_ShowInventory,
  GameState_ShowMap,
} GameState;

typedef enum {
  DialogState_None,
  DialogState_InProgress,
  DialogState_Done,
} DialogState;

#define INVENTORY_TYPES_NUM 7
static const uint8_t inventoryTypeForId[] = {0, 1, 2, 6, 3, 1, 1, 3, 5, 4};

typedef struct _GameContext {
  Display *display;
  AudioSystem audio;
  GameState state;
  GameState prevState;

  Menu *currentMenu;

  uint16_t currentBock;
  Orientation orientation;

  LevelContext *level;
  int levelId;

  Language language;
  LangHandle lang;

  // party
  SAVCharacter chars[NUM_CHARACTERS];
  uint8_t selectedChar;
  uint8_t selectedCharIsCastingSpell;
  uint16_t inventory[INVENTORY_SIZE];
  uint16_t inventoryIndex;
  uint16_t itemIndexInHand;
  uint16_t credits;

  INFScript script;
  uint16_t nextFunc;

  EMCInterpreter interp;
  EMCState interpState;

  GameEngine *engine;

  GameTimInterpreter timInterpreter;
  Animator animator;

  ItemProperty *itemProperties;
  GameObject *itemsInGame;
  uint16_t itemsCount;

  int _shouldRun;

  DialogState dialogState;

  char *savDir;

  PAKFile sfxPak;
  PAKFile defaultTlkFile; // 00.TLK
  GameConfig conf;

  const SpellProperties *spellProperties; // count is SPELL_PROPERTIES_COUNT
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

int GameContextNewGame(GameContext *gameCtx, int selectedChar);

int GameContextLoadLevel(GameContext *ctx, int levelNum);
void GameContextLoadLevelShapes(GameContext *gameCtx, const char *shpFile,
                                const char *datFile);
int GameContextLoadChars(GameContext *ctx);
int GameContextRunScript(GameContext *gameCtx, int function);
int GameContextRunItemScript(GameContext *gameCtx, uint16_t charId,
                             uint16_t itemId, uint16_t flags, uint16_t next,
                             uint16_t reg4);

void GameContextSetState(GameContext *gameCtx, GameState newState);

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId);
uint8_t GameContextGetNumChars(const GameContext *ctx);

uint16_t GameContextGetString(const GameContext *ctx, uint16_t stringId,
                              char *outBuffer, size_t outBufferSize);

// string needs to be freed!
char *GameContextGetString2(const GameContext *ctx, uint16_t stringId);
uint16_t GameContextGetLevelName(const GameContext *gameCtx, char *outBuffer,
                                 size_t outBufferSize);

// returns a string with %n replaced with hero name. Needs to be freed!
char *GameContextGetString3(const GameContext *ctx, uint16_t stringId);

uint16_t GameContextGetItemSHPFrameIndex(GameContext *gameCtx, uint16_t itemId);

void GameContextInitSceneDialog(GameContext *gameCtx);
void GameContextCleanupSceneDialog(GameContext *gameCtx);

uint16_t GameContextCreateItem(GameContext *gameCtx, uint16_t itemType);
void GameContextDeleteItem(GameContext *gameCtx, uint16_t itemIndex);

void GameContextUpdateCursor(GameContext *gameCtx);
void GameContextExitGame(GameContext *gameCtx);
int GameContextLoadSaveFile(GameContext *gameCtx, const char *filepath);

void GameContextLoadTLKFile(GameContext *gameCtx, int levelIndex);
void GameContextPlayDialogSpeech(GameContext *gameCtx, int16_t charId,
                                 uint16_t soundID);
void GameContextPlaySoundFX(GameContext *gameCtx, uint16_t soundId);

typedef enum {
  ButtonType_Up,
  ButtonType_Down,
  ButtonType_Left,
  ButtonType_Right,
  ButtonType_TurnLeft,
  ButtonType_TurnRight,

  ButtonType_Automap,
} ButtonType;

void GameContextButtonClicked(GameContext *gameCtx, ButtonType button);

uint16_t GetRandom(uint16_t min, uint16_t max);

int GameContextCheckMagic(GameContext *gameCtx, uint16_t charId,
                          uint16_t spellNum, uint16_t spellLevel);
