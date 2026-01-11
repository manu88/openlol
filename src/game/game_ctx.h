#pragma once
#include "SDL_render.h"
#include "animator.h"
#include "audio.h"
#include "config.h"
#include "formats/format_cps.h"
#include "formats/format_fnt.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "game_tim_animator.h"
#include "geometry.h"
#include "level.h"
#include "menu.h"
#include "monster.h"
#include "pak_file.h"
#include "script.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_FACTOR 4

#define PIX_BUF_WIDTH 320
#define PIX_BUF_HEIGHT 200

#define DIALOG_BUFFER_SIZE (size_t)1024

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

typedef enum {
  GameState_Invalid,
  GameState_MainMenu,
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
  int showBitmap;
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

  Menu *currentMenu;
  int shouldUpdate;

  char *savDir;

  PAKFile sfxPak;

  GameConfig conf;
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
void GameContextLoadLevelShapes(GameContext *gameCtx, const char *shpFile,
                                const char *datFile);
int GameContextLoadChars(GameContext *ctx);
int GameContextRunLevelInitScript(GameContext *gameCtx);
int GameContextRunScript(GameContext *gameCtx, int function);
int GameContextRunItemScript(GameContext *gameCtx, uint16_t charId,
                             uint16_t itemId, uint16_t flags, uint16_t next,
                             uint16_t reg4);

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

void GameContextLoadBackgroundInventoryIfNeeded(GameContext *gameCtx,
                                                int charId);

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
