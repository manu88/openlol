#include "menu.h"
#include "geometry.h"
#include "ui.h"
#include <assert.h>
#include <string.h>

static GameMenu _gameMenu = {0};
static GameMenu _mainMenu = {0};

GameMenu *gameMenu = &_gameMenu;
GameMenu *mainMenu = &_mainMenu;

void GameMenuReset(GameMenu *menu) {
  menu->returnToGame = 0;
  menu->state = GameMenuState_GameMenu;
}
static void Render_ExitGame(GameMenu *menu, const FNTHandle *font,
                            SDL_Texture *pixBuf) {
  int startX = 16;
  int startY = 72;
  int width = 288;
  int height = 52;
  UIDrawMenuWindow(pixBuf, startX, startY, width, height);
  UIRenderTextCentered(font, pixBuf, startX + width / 2, startY + 8,
                       "Do you really want to exit the kingdom?");
  UIDrawButton(font, pixBuf, startX + 8, startY + 30, 72, 15, "yes");
  UIDrawButton(font, pixBuf, startX + 208, startY + 30, 72, 15, "no");
}

static void Render_MainMenu(GameMenu *menu, const FNTHandle *font,
                            SDL_Texture *pixBuf) {
  assert(menu == gameMenu);
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

void GameMenuRender(GameMenu *menu, const FNTHandle *font,
                    SDL_Texture *pixBuf) {
  UISetStyle(UIStyle_GameMenu);
  switch (menu->state) {

  case GameMenuState_GameMenu:
    Render_MainMenu(menu, font, pixBuf);
    break;
  case GameMenuState_LoadGame:
    printf("Render LoadGame\n");
    break;
  case GameMenuState_SaveGame:
    printf("Render SaveGame\n");
    break;
  case GameMenuState_DeleteGame:
    printf("Render DeleteGame\n");
    break;
  case GameMenuState_GameControls:
    printf("Render Game controls\n");
    break;
  case GameMenuState_AudioControls:
    printf("Render Audio controls\n");
    break;
  case GameMenuState_ExitGame:
    Render_ExitGame(menu, font, pixBuf);
    break;
  }
}

static int Mouse_ExitGame(GameMenu *menu, const Point *pt) {

  int startX = 16;
  int startY = 72;

  if (zoneClicked(pt, startX + 8, startY + 30, 72, 15)) {
    printf("YES\n");
    return 1;
  } else if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    printf("NO\n");
    menu->state = GameMenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int Mouse_MainMenu(GameMenu *menu, const Point *pt) {
  int buttonY = GAME_MENU_BUTTONS_START_Y;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Load game\n");
    menu->state = GameMenuState_LoadGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Save this game\n");
    menu->state = GameMenuState_SaveGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Delete a game\n");
    menu->state = GameMenuState_DeleteGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Game controls\n");
    menu->state = GameMenuState_GameControls;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Audio controls\n");
    menu->state = GameMenuState_AudioControls;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Exit Game\n");
    menu->state = GameMenuState_ExitGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTON_H + 4;
  if (zoneClicked(pt, GAME_MENU_RESUME_BUTTON_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_RESUME_BUTTON_W, GAME_MENU_BUTTON_H)) {
    menu->returnToGame = 1;
    printf("resume\n");
    return 1;
  }

  return 0;
}

int GameMenuMouse(GameMenu *menu, const Point *pt) {
  switch (menu->state) {

  case GameMenuState_GameMenu:
    return Mouse_MainMenu(menu, pt);
  case GameMenuState_LoadGame:
  case GameMenuState_SaveGame:
  case GameMenuState_DeleteGame:
  case GameMenuState_GameControls:
  case GameMenuState_AudioControls:
  case GameMenuState_ExitGame:
    return Mouse_ExitGame(menu, pt);
    break;
  }
  return 0;
}

int GameMenuKeyDown(GameMenu *menu, const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_ESCAPE:
    if (menu->state == GameMenuState_GameMenu) {
      menu->returnToGame = 1;
    } else {
      menu->state = GameMenuState_GameMenu;
    }
    return 1;
  }
  return 0;
}
