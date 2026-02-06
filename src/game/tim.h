#pragma once
#include "game_ctx.h"
#include <stdint.h>

void TIMLoad(uint16_t scriptId, const char *file);
void TIMRun(GameContext *gameCtx, uint16_t scriptId, uint16_t loop);
void TIMRelease(uint16_t scriptId);

void TimLoadWSA(GameContext *gameCtx,uint16_t index, const char *wsaFile, int x, int y,
                int offscreen, int flags);
