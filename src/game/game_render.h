#pragma once
#include "game_ctx.h"

void GameRender(GameContext *gameCtx);

void GameCopyPage(GameContext *gameCtx, uint16_t srcX, uint16_t srcY,
                  uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                  uint16_t srcPage, uint16_t dstPage);
