#pragma once
#include "game_ctx.h"

void renderBackground(GameContext *gameCtx);
void GameRenderMap(GameContext *gameCtx, int xOff, int yOff);
void GameRenderScene(GameContext *gameCtx);
void clearMazeZone(GameContext *gameCtx);
void renderCPS(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
               const uint8_t *paletteBuffer, int w, int h);
void renderCPSAt(SDL_Texture *pixBuf, const uint8_t *imgData, size_t dataSize,
               const uint8_t *paletteBuffer,int x, int y, int w, int h);
