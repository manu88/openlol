#pragma once
#include "game_ctx.h"

void renderTextStats(GameContext *gameCtx);
void renderDialog(GameContext *gameCtx);
void renderText(GameContext *gameCtx, int xOff, int yOff, int width, const char *text);
