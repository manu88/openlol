#pragma once
#include "game_ctx.h"
#include <stdint.h>

void TIMLoad(uint16_t scriptId, const char *file);
void TIMRun(GameContext *gameCtx, uint16_t scriptId, uint16_t loop);
void TIMRelease(uint16_t scriptId);

void TimLoadWSA(GameContext *gameCtx, uint16_t index, const char *wsaFile,
                int x, int y, int offscreen, int flags);

void TimSetupPart(GameContext *gameCtx, uint16_t animIndex, uint16_t partIndex,
                  uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles,
                  uint16_t nextPart, uint16_t partDelay, uint16_t field,
                  uint16_t sfxIndex, uint16_t sfxFrame);
void TimStartPart(GameContext *gameCtx, uint16_t animIndex, uint16_t partIndex);
