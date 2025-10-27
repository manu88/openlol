#include "ui.h"
#include "SDL_pixels.h"
#include "renderer.h"

static UIStyle _currentStyle = UIStyle_Default;
void UISetStyle(UIStyle style) { _currentStyle = style; }

typedef struct {
  SDL_Color background;
  SDL_Color topLayerColor;
  SDL_Color bottomLayerColor;

  SDL_Color textColorMap[2];
} UIPalette;

static UIPalette gameMenuPalette = {.background = {166, 89, 77},
                                    .topLayerColor = {207, 125, 101},
                                    .bottomLayerColor = {121, 52, 48},
                                    .textColorMap = {
                                        {166, 89, 77},   // background
                                        {247, 207, 121}, // font color
                                    }};

static UIPalette defaultPalette = {.background = {65, 44, 36},
                                   .topLayerColor = {207, 125, 101},
                                   .bottomLayerColor = {119, 50, 46},
                                   .textColorMap = {
                                       {65, 44, 36},    // background
                                       {255, 255, 255}, // font color
                                   }};

static UIPalette *getPalette(void) {
  switch (_currentStyle) {

  case UIStyle_Default:
    return &defaultPalette;
  case UIStyle_GameMenu:
    return &gameMenuPalette;
    break;
  }
  assert(0);
}

static void DrawChar(SDL_Texture *pixBuf, const FNTHandle *font, uint16_t c,
                     int xOff, int yOff) {
  assert(pixBuf);
  const UIPalette *pal = getPalette();
  if (c >= font->numGlyphs) {
    return;
  }

  if (!font->bitmapOffsets[c]) {
    return;
  }
  const uint8_t *src = font->buffer + font->bitmapOffsets[c];
  const uint8_t charWidth = font->widthTable[c];

  if (!charWidth)
    return;

  void *data;
  int pitch;
  SDL_LockTexture(pixBuf, NULL, &data, &pitch);

  uint8_t charH1 = font->heightTable[c * 2 + 0];
  uint8_t charH2 = font->heightTable[c * 2 + 1];
  uint8_t charH0 = font->maxHeight - (charH1 + charH2);

  int x = xOff;
  int y = yOff;
  while (charH1--) {
    SDL_Color col = pal->textColorMap[0];
    for (int i = 0; i < charWidth; ++i) {
      drawPix(data, pitch, col.r, col.g, col.b, x, y);
      x++;
    }
    x = xOff;
    y += 1;
  }

  while (charH2--) {
    uint8_t b = 0;
    for (int i = 0; i < charWidth; ++i) {
      SDL_Color col;
      if (i & 1) {
        col = pal->textColorMap[b >> 4];
      } else {
        b = *src++;
        col = pal->textColorMap[b & 0xF];
      }

      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      row[x] = 0XFF000000 + (col.r << 0X10) + (col.g << 0X8) + col.b;
      x += 1;
    }
    y += 1;
    x = xOff;
  }

  while (charH0--) {
    SDL_Color col = pal->textColorMap[0];
    for (int i = 0; i < charWidth; ++i) {
      uint32_t *row = (unsigned int *)((char *)data + pitch * y);
      row[x] = 0XFF000000 + (col.r << 0X10) + (col.g << 0X8) + col.b;
      x += 1;
    }
    y += 1;
    x = xOff;
  }
  SDL_UnlockTexture(pixBuf);
}

void UIRenderText(GameContext *gameCtx, SDL_Texture *texture, int xOff,
                  int yOff, int width, const char *text) {
  if (!text) {
    return;
  }
  int x = xOff;
  int y = yOff;
  for (int i = 0; i < strlen(text); i++) {
    if (!isprint(text[i])) {
      continue;
    }
    DrawChar(texture, &gameCtx->defaultFont, text[i], x, y);
    x += gameCtx->defaultFont.widthTable[(uint8_t)text[i]];
    if (x - xOff >= width) {
      x = xOff;
      y += gameCtx->defaultFont.maxHeight + 2;
    }
  }
}

void UIRenderTextCentered(GameContext *gameCtx, SDL_Texture *texture,
                          int xCenter, int yCenter, const char *text) {
  if (!text) {
    return;
  }
  int width = 0;
  for (int i = 0; i < strlen(text); i++) {
    if (!isprint(text[i])) {
      continue;
    }
    width += gameCtx->defaultFont.widthTable[(uint8_t)text[i]];
  }
  UIRenderText(gameCtx, texture, xCenter - (width / 2), yCenter, width, text);
}

void UIDrawButton(GameContext *gameCtx, SDL_Texture *texture, int x, int y,
                  int w, int h, const char *text) {
  const UIPalette *pal = getPalette();
  void *data;
  int pitch;
  SDL_Rect rect = {x, y, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      drawPix(data, pitch, pal->background.r, pal->background.g,
              pal->background.b, x, y);
      if ((y == 0 && x != 0)) {
        drawPix(data, pitch, pal->topLayerColor.r, pal->topLayerColor.g,
                pal->topLayerColor.b, x, y); // top border
      } else if (x == w - 1) {
        drawPix(data, pitch, pal->bottomLayerColor.r, pal->bottomLayerColor.g,
                pal->bottomLayerColor.b, x, y); // right border
      } else if (x == 0) {
        drawPix(data, pitch, pal->topLayerColor.r, pal->topLayerColor.g,
                pal->topLayerColor.b, x, y); // left border

      } else if (y == h - 1) {
        drawPix(data, pitch, pal->bottomLayerColor.r, pal->bottomLayerColor.g,
                pal->bottomLayerColor.b, x, y); // bottom border
      }
    }
  }
  SDL_UnlockTexture(texture);
  UIRenderTextCentered(gameCtx, texture, x + w / 2, (y + h / 2) - 2, text);
}

void UIDrawMenuWindow(GameContext *gameCtx, SDL_Texture *texture, int startX,
                      int startY, int w, int h) {

  const UIPalette *pal = getPalette();
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      drawPix(data, pitch, pal->background.r, pal->background.g,
              pal->background.b, x, y);

      if ((y < 2) || (x < 2)) {
        drawPix(data, pitch, pal->topLayerColor.r, pal->topLayerColor.g,
                pal->topLayerColor.b, x, y);
      }
      if ((y >= h - 2) || (x >= w - 2)) {
        drawPix(data, pitch, pal->bottomLayerColor.r, pal->bottomLayerColor.g,
                pal->bottomLayerColor.b, x, y);
      }
    }
  }
  SDL_UnlockTexture(texture);
}
