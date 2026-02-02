#pragma once
#include "game_ctx.h"
#include <stdint.h>

void GameRenderMaze(GameContext *gameCtx);

void renderCPSAt(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
                 const uint8_t *paletteBuffer, int destX, int destY,
                 int sourceW, int sourceH, int imageW, int imageH);
