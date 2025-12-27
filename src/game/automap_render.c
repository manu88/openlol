#include "automap_render.h"
#include "render.h"
#include "ui.h"
void AutomapRender(GameContext *gameCtx) {
  UISetDefaultStyle();
  renderCPS(gameCtx->pixBuf, gameCtx->mapBackground.data,
            gameCtx->mapBackground.imageSize, gameCtx->mapBackground.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  char c[20] = "";
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf,
               MAP_SCREEN_EXIT_BUTTON_X + 2, MAP_SCREEN_BUTTONS_Y + 4, 50, c);

  GameContextGetLevelName(gameCtx, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, MAP_SCREEN_NAME_X,
               MAP_SCREEN_NAME_Y, 320 - MAP_SCREEN_NAME_Y, c);
  printf("%i\n", gameCtx->levelId);
}
