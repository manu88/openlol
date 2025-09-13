#pragma once
#include "format_cmz.h"
#include "format_dat.h"
#include "format_shp.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  Point coords;
  uint8_t valid;
} ViewConeEntry;

#define VIEW_CONE_NUM_CELLS 17

typedef enum {
  North = 0,
  East = 1,
  South = 2,
  West = 3,
} Orientation;

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
