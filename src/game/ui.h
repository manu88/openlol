#pragma once
#include "formats/format_fnt.h"
#include <SDL2/SDL.h>
typedef enum {
  UIStyle_Default,
  UIStyle_GameMenu,
} UIStyle;

void UISetStyle(UIStyle style);
static inline void UISetDefaultStyle(void) { UISetStyle(UIStyle_Default); }

void UIRenderText(const FNTHandle *font, SDL_Texture *texture, int xOff,
                  int yOff, int width, const char *text);
void UIRenderTextCentered(const FNTHandle *font, SDL_Texture *texture,
                          int xCenter, int yCenter, const char *text);
void UIDrawButton(const FNTHandle *font, SDL_Texture *texture, int x, int y,
                  int w, int h, const char *text);
void UIDrawMenuWindow(SDL_Texture *texture, int x, int y, int w, int h);
