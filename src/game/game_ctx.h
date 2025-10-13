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

typedef struct {
  uint16_t stringId;
  uint16_t shapeId;
} Item;

typedef struct {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  MazeHandle mazHandle;
  WllHandle wllHandle;
  DatHandle datHandle;
  SHPHandle shpHandle;
  LangHandle levelLang;
} LevelContext;

typedef enum {
  GameState_PlayGame = 0,
  GameState_TimAnimation = 1,
  GameState_ShowInventory = 2,
} GameState;

typedef struct {
  Point pos;
  uint8_t pending;
  uint8_t isRightClick;
} MouseEvent;

typedef struct _GameContext {
  GameState state;
  int fadeOutFrames;
  int dialogBoxFrames;
  int showBigDialog;

  MouseEvent mouseEv;
  uint16_t currentBock;
  Point partyPos;
  Orientation orientation;

  LevelContext *level;

  SDL_Texture *pixBuf;
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

  EMCInterpreter interp;
  EMCState interpState;

  char *dialogTextBuffer;
  char *dialogText; // will either be NULL or pointing to dialogTextBuffer

  FNTHandle defaultFont;
  uint8_t gameFlags[100];

  GameTimAnimator timAnimator;

  uint16_t inventory[INVENTORY_SIZE];
  uint16_t inventoryIndex;
  uint16_t itemInHand;

  uint8_t *defaultPalette;

  int controlDisabled;

  SDL_Cursor *cursor;

  Item *items;
  uint16_t itemsCount;
} GameContext;

void GameContextRelease(GameContext *gameCtx);
int GameContextInit(GameContext *gameCtx);
int GameContextStartup(GameContext *ctx);

int GameContextLoadLevel(GameContext *ctx, int levelNum, uint16_t startBlock,
                         uint16_t startDir);
int GameContextRunLevelInitScript(GameContext *gameCtx);
int GameContextRunScript(GameContext *gameCtx, int function);

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag);
void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag, uint16_t val);
void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag);

void GameContextSetState(GameContext *gameCtx, GameState newState);

int GameContextAddItemToInventory(GameContext *ctx, uint16_t itemId);
uint8_t GameContextGetNumChars(const GameContext *ctx);
