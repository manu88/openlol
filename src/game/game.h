#pragma once
#include "game_ctx.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void LevelContextRelease(LevelContext *levelCtx);
int cmdGame(int argc, char *argv[]);
