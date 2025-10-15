#pragma once
#include "SDL_render.h"
#include "game_ctx.h"

void renderDialog(GameContext *gameCtx);
void renderText(GameContext *gameCtx, SDL_Texture* texture, int xOff, int yOff, int width,
                const char *text);

void renderPlayField(GameContext *gameCtx);
void GameRender(GameContext *gameCtx);

void GameCopyPage(GameContext *gameCtx, uint16_t srcX, uint16_t srcY,
                  uint16_t destX, uint16_t destY, uint16_t w, uint16_t h,
                  uint16_t srcPage, uint16_t dstPage);
