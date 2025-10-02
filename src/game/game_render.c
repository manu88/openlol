#include "game_render.h"

static void renderStatLine(GameContext *gameCtx, const char *line, int x,
                           int y) {
  if (strlen(line) == 0) {
    return;
  }
  gameCtx->textSurface = TTF_RenderUTF8_Solid(gameCtx->font, line,
                                              (SDL_Color){255, 255, 255, 255});

  gameCtx->textTexture =
      SDL_CreateTextureFromSurface(gameCtx->renderer, gameCtx->textSurface);
  SDL_Rect dstrect = {x, y, gameCtx->textSurface->w, gameCtx->textSurface->h};
  SDL_RenderCopy(gameCtx->renderer, gameCtx->textTexture, NULL, &dstrect);
  SDL_DestroyTexture(gameCtx->textTexture);
  SDL_FreeSurface(gameCtx->textSurface);
}

void renderDialog(GameContext *gameCtx) {
  if (gameCtx->dialogText) {
    renderStatLine(gameCtx, gameCtx->dialogText, 30, 250);
  }
}

static char textStatsBuffer[512];
void renderTextStats(GameContext *gameCtx) {
  int statsPosX = 20;
  int statsPosY = 400;

  uint16_t gameX = 0;
  uint16_t gameY = 0;
  GetGameCoords(gameCtx->partyPos.x, gameCtx->partyPos.y, &gameX, &gameY);
  snprintf(textStatsBuffer, sizeof(textStatsBuffer),
           "pose x=%i (%X) y=%i (%X) o=%i %c", gameCtx->partyPos.x, gameX,
           gameCtx->partyPos.y, gameY, gameCtx->orientation,
           gameCtx->orientation == North
               ? 'N'
               : (gameCtx->orientation == South  ? 'S'
                  : gameCtx->orientation == East ? 'E'
                                                 : 'W'));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  gameCtx->currentBock = BlockFromCoords(gameX, gameY);
  snprintf(textStatsBuffer, sizeof(textStatsBuffer),
           "block: %X  newblockpos: %X", gameCtx->currentBock,
           BlockCalcNewPosition(gameCtx->currentBock, gameCtx->orientation));
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  snprintf(textStatsBuffer, sizeof(textStatsBuffer), "console mode: %i",
           gameCtx->consoleHasFocus);
  renderStatLine(gameCtx, textStatsBuffer, statsPosX, statsPosY);

  statsPosY += 20;
  renderStatLine(gameCtx, gameCtx->cmdBuffer, statsPosX, statsPosY);
}
