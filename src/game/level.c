#include "level.h"
#include <string.h>

void LevelInit(LevelContext *level) {
  memset(level, 0, sizeof(LevelContext));
  level->currentTlkFileIndex = -1;
}
