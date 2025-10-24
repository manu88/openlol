#include "ui.h"
#include "renderer.h"

void renderText(GameContext *gameCtx, SDL_Texture *texture, int xOff, int yOff,
                int width, const char *text) {
  if (!text) {
    return;
  }
  int x = xOff;
  int y = yOff;
  for (int i = 0; i < strlen(text); i++) {
    if (!isprint(text[i])) {
      continue;
    }
    drawChar2(texture, &gameCtx->defaultFont, text[i], x, y);
    x += gameCtx->defaultFont.widthTable[(uint8_t)text[i]];
    if (x - xOff >= width) {
      x = xOff;
      y += gameCtx->defaultFont.maxHeight + 2;
    }
  }
}

void renderTextCentered(GameContext *gameCtx, SDL_Texture *texture, int xCenter,
                        int yCenter, const char *text) {
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
  renderText(gameCtx, texture, xCenter - (width / 2), yCenter, width, text);
}

void drawButton(GameContext *gameCtx, SDL_Texture *texture, int x, int y, int w,
                int h, const char *text) {
  void *data;
  int pitch;
  SDL_Rect rect = {x, y, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      drawPix(data, pitch, 60, 45, 37, x, y);
      if ((y == 0 && x != 0) || (x == w - 1)) {
        drawPix(data, pitch, 120, 120, 120, x, y);
      } else if ((x == 0) || (y == h - 1)) {
        drawPix(data, pitch, 20, 25, 17, x, y);
      }
    }
  }
  SDL_UnlockTexture(texture);

  renderText(gameCtx, texture, x + 4, y + 2, w, text);
}

void drawMenuWindow(GameContext *gameCtx, SDL_Texture *texture, int startX,
                    int startY, int w, int h) {
  void *data;
  int pitch;
  SDL_Rect rect = {startX, startY, w, h};
  SDL_LockTexture(texture, &rect, &data, &pitch);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      drawPix(data, pitch, 174, 117, 60, x, y);

      if ((y < 2) || (x < 2)) {
        drawPix(data, pitch, 247, 182, 105, x, y);
      }
      if ((y >= h - 2) || (x >= w - 2)) {
        drawPix(data, pitch, 97, 69, 52, x, y);
      }
    }
  }
  SDL_UnlockTexture(texture);
}
