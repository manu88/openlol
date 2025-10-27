#include "menu.h"
#include "geometry.h"
#include "ui.h"
#include <assert.h>
#include <string.h>

static GameMenu _gameMenu = {0};
static GameMenu _mainMenu = {0};

GameMenu *gameMenu = &_gameMenu;
GameMenu *mainMenu = &_mainMenu;

void GameMenuRender(const FNTHandle *font, SDL_Texture *pixBuf,
                    GameMenu *menu) {
  assert(menu == gameMenu);
  UISetStyle(UIStyle_GameMenu);
  UIDrawMenuWindow(pixBuf, GAME_MENU_X, GAME_MENU_Y, GAME_MENU_W, GAME_MENU_H);

  UIRenderTextCentered(font, pixBuf, GAME_MENU_X + GAME_MENU_W / 2,
                       GAME_MENU_Y + 10, "Lands of Lore");

  int buttonY = GAME_MENU_BUTTONS_START_Y;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Load a game");

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Save this game");

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Delete a game");

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Game controls");

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Audio controls");

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, "Exit Game");

  buttonY += GAME_MENU_BUTTON_H + 4;
  UIDrawButton(font, pixBuf, GAME_MENU_RESUME_BUTTON_X, GAME_MENU_Y + buttonY,
               GAME_MENU_RESUME_BUTTON_W, GAME_MENU_BUTTON_H, "Resume Game");
}
