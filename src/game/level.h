#pragma once
#include "formats/format_cmz.h"
#include "formats/format_dat.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_tlc.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "formats/format_xxx.h"
#include "monster.h"
#include "pak_file.h"
#include <stdint.h>

#define MAX_MONSTER_PROPERTIES 5

typedef struct {
  uint8_t walls[4];
  uint8_t direction;
  uint16_t flags;
} BlockProperty;

typedef struct _LevelContext {
  VCNHandle vcnHandle;
  VMPHandle vmpHandle;
  TLCHandle tlcHandle;
  WllHandle wllHandle;
  DatHandle datHandle;
  SHPHandle shpHandle;
  LangHandle levelLang;

  SHPHandle doors;
  SHPHandle monsterShapes[MAX_MONSTERS];

  MonsterProperties monsterProperties[MAX_MONSTER_PROPERTIES];
  Monster monsters[MAX_MONSTERS];

  PAKFile currentTlkFile;
  int currentTlkFileIndex;

  BlockProperty blockProperties[MAZE_NUM_CELL];
  XXXHandle legendData;

} LevelContext;

void LevelInit(LevelContext *level);
