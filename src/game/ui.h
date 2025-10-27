#pragma once
#include "game_ctx.h"

typedef enum {
  UIStyle_Default,
  UIStyle_GameMenu,
} UIStyle;

void UISetStyle(UIStyle style);
static inline void UISetDefaultStyle(void) { UISetStyle(UIStyle_Default); }

void UIRenderText(GameContext *gameCtx, SDL_Texture *texture, int xOff,
                  int yOff, int width, const char *text);
void UIRenderTextCentered(GameContext *gameCtx, SDL_Texture *texture,
                          int xCenter, int yCenter, const char *text);
void UIDrawButton(GameContext *gameCtx, SDL_Texture *texture, int x, int y,
                  int w, int h, const char *text);
void UIDrawMenuWindow(GameContext *gameCtx, SDL_Texture *texture, int x, int y,
                      int w, int h);
