#pragma once
#include "game_ctx.h"

void renderText(GameContext *gameCtx, SDL_Texture *texture, int xOff, int yOff,
                int width, const char *text);
void renderTextCentered(GameContext *gameCtx, SDL_Texture *texture, int xCenter, int yCenter,
                 const char *text);
void drawButton(GameContext *gameCtx, SDL_Texture *texture, int x, int y, int w,
                int h, const char *text);
void drawMenuWindow(GameContext *gameCtx, SDL_Texture *texture, int x, int y,
                    int w, int h);
