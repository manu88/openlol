#pragma once

#include "SDL_render.h"
#include "format_cmz.h"
#include "format_dat.h"
#include "format_shp.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include "geometry.h"
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

  Point partyPos;
  Orientation orientation;

  ViewConeEntry viewConeEntries[VIEW_CONE_NUM_CELLS];
} LevelContext;

typedef struct {
  SDL_Renderer *renderer;
  SDL_Window *window;

  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  TTF_Font *font;
} GameContext;

void GameContextRelease(GameContext *gameCtx);
void LevelContextRelease(LevelContext *levelCtx);
int cmdGame(int argc, char *argv[]);
