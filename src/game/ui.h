#pragma once
#include "formats/format_fnt.h"
#include <SDL2/SDL.h>

typedef enum {
  UIStyle_Default,
  UIStyle_GameMenu,
  UIStyle_MainMenu,
  UIStyle_Inventory,
  UIStyle_ManaLifeBars,
} UIStyle;

typedef enum {
  UITextStyle_Default,
  UITextStyle_Highlighted,
} UITextStyle;

void UISetStyle(UIStyle style);
UIStyle UIGetCurrentStyle(void);
void UISetTextStyle(UITextStyle textStyle);
void UIResetTextStyle(void);

static inline void UISetDefaultStyle(void) {
  UISetStyle(UIStyle_Default);
  UIResetTextStyle();
}

void UIRenderText(const FNTHandle *font, SDL_Texture *texture, int xOff,
                  int yOff, int width, const char *text);
void UIRenderTextCentered(const FNTHandle *font, SDL_Texture *texture,
                          int xCenter, int yCenter, const char *text);
void UIRenderTextLeft(const FNTHandle *font, SDL_Texture *texture, int xTopLeft,
                      int yTopLeft, const char *text);
void UIDrawTextButton(const FNTHandle *font, SDL_Texture *texture, int x, int y,
                      int w, int h, const char *text);
void UIDrawButton(SDL_Texture *texture, int x, int y, int w, int h);
void UIDrawMenuWindow(SDL_Texture *texture, int x, int y, int w, int h);

void UIStrokeRect(SDL_Texture *texture, int startX, int startY, int w, int h,
                  SDL_Color col);
void UIFillRect(SDL_Texture *texture, int startX, int startY, int w, int h,
                SDL_Color col);
