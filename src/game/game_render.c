#include "game_render.h"
#include "renderer.h"
#include <stdint.h>
#include <string.h>

void renderText(GameContext *gameCtx, int xOff, int yOff, int width,
                const char *text) {
  if (!text) {
    return;
  }
  int x = xOff;
  int y = yOff;
  for (int i = 0; i < strlen(text) - 1; i++) {

    drawChar2(gameCtx->pixBuf, &gameCtx->defaultFont, text[i], x, y);
    x += gameCtx->defaultFont.widthTable[(uint8_t)text[i]];
    if (x - xOff >= width) {
      x = xOff;
      y += gameCtx->defaultFont.maxHeight + 2;
    }
  }
}

void renderDialog(GameContext *gameCtx) {
  if (gameCtx->dialogText) {
    renderText(gameCtx, DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_W,
               gameCtx->dialogText);
  }
}
