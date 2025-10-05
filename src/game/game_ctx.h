#pragma once
#include "SDL_render.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_fnt.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "geometry.h"
#include "pak_file.h"
#include "script.h"
#include "tim_game_animator.h"
#include <stdint.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

#define PIX_BUF_WIDTH 320
#define PIX_BUF_HEIGHT 200

#define DIALOG_BUFFER_SIZE (size_t)1024

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
} GameState;

typedef struct _GameContext {
  GameState state;
  uint16_t currentBock;
  Point partyPos;
  Orientation orientation;

  LevelContext *level;

  SDL_Texture *pixBuf;
  SDL_Renderer *renderer;
  SDL_Window *window;
  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];

  CPSImage playField;
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

} GameContext;

void GameContextRelease(GameContext *gameCtx);
int GameContextInit(GameContext *gameCtx);

int GameContextLoadLevel(GameContext *ctx, int levelNum, uint16_t startBlock,
                         uint16_t startDir);
int GameContextRunLevelInitScript(GameContext *gameCtx);
int GameContextRunScript(GameContext *gameCtx, int function);

uint16_t GameContextGetGameFlag(const GameContext *gameCtx, uint16_t flag);
void GameContextSetGameFlag(GameContext *gameCtx, uint16_t flag, uint16_t val);
void GameContextResetGameFlag(GameContext *gameCtx, uint16_t flag);

void GameContextSetState(GameContext *gameCtx, GameState newState);
