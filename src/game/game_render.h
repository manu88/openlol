#pragma once
#include "game_ctx.h"
#include <stdint.h>

void GameRender(GameContext *gameCtx);
void GameRenderRenderSceneFade(GameContext *gameCtx);

int GameRenderRenderExpandDialogBox(GameContext *gameCtx);
int GameRenderRenderShrinkDialogBox(GameContext *gameCtx);

void GameCopyPage(GameContext *gameCtx, uint16_t srcX, uint16_t srcY,
                  uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                  uint16_t srcPage, uint16_t dstPage);

void GameRenderResetDialog(GameContext *gameCtx);

void GameRenderSetDialogF(GameContext *gameCtx, int stringId, ...);
