#pragma once
#include "game_ctx.h"

void renderDialog(GameContext *gameCtx);
void renderText(GameContext *gameCtx, int xOff, int yOff, int width,
                const char *text);
