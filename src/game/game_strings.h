#pragma once
#include "game_ctx.h"



void GameContextSetDialogF(GameContext *gameCtx, int stringId, ...);
char *stringReplaceHeroNameAt(const GameContext *gameCtx, char *string,
                              size_t bufferSize, int index);
int stringHasCharName(const char *s, int startIndex);
