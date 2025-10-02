#pragma once

#include "SDL_render.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "geometry.h"
#include "pak_file.h"
#include "script.h"
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  MazeHandle mazHandle;
  WllHandle wllHandle;
  DatHandle datHandle;
  SHPHandle shpHandle;
  LangHandle levelLang;

  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];
} LevelContext;

typedef struct _GameContext {

  uint16_t currentBock;
  Point partyPos;
  Orientation orientation;

  LevelContext *level;
  SDL_Renderer *renderer;
  SDL_Window *window;

  CPSImage playField;
  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  TTF_Font *font;
  PAKFile generalPak;

  INFScript script;
  INFScript iniScript;
  EMCInterpreter interp;

  // dev/debug things
  int consoleHasFocus;
  char *cmdBuffer;
} GameContext;

void GameContextRelease(GameContext *gameCtx);
int GameInitContext(GameContext *gameCtx);
void LevelContextRelease(LevelContext *levelCtx);
int cmdGame(int argc, char *argv[]);
int runScript(GameContext *gameCtx, int function);
