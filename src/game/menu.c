#include "menu.h"
#include "SDL_keycode.h"
#include "formats/format_lang.h"
#include "game_ctx.h"
#include "geometry.h"
#include "render.h"
#include "ui.h"
#include <assert.h>
#include <string.h>

static GameMenu _gameMenu = {0};
static MainMenu _mainMenu = {0};

Menu *gameMenu = (Menu *)&_gameMenu;
static void GameMenuReset(GameMenu *menu);
static void GameMenuRender(GameMenu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf);
static int GameMenuMouse(GameMenu *menu, GameContext *context, const Point *pt);
static int GameMenuKeyDown(GameMenu *menu, GameContext *context,
                           const SDL_Event *e);

Menu *mainMenu = (Menu *)&_mainMenu;
static void MainMenuReset(MainMenu *menu);
static void MainMenuRender(MainMenu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf);
static int MainMenuMouse(MainMenu *menu, GameContext *context, const Point *pt);
static int MainMenuKeyDown(MainMenu *menu, GameContext *context,
                           const SDL_Event *e);

static char textBuf[128] = "";

void MenuReset(Menu *menu) {
  menu->returnToGame = 0;
  menu->selectedIndex = 0;
  if (menu == gameMenu) {
    GameMenuReset((GameMenu *)menu);
  } else if (menu == mainMenu) {
    MainMenuReset((MainMenu *)menu);
  } else {
    assert(0);
  }
}

void MenuRender(Menu *menu, GameContext *context, const FNTHandle *font,
                SDL_Texture *pixBuf) {
  assert(menu);
  if (menu == gameMenu) {
    GameMenuRender((GameMenu *)menu, context, font, pixBuf);
  } else if (menu == mainMenu) {
    MainMenuRender((MainMenu *)menu, context, font, pixBuf);
  } else {
    assert(0);
  }
}

int MenuMouse(Menu *menu, GameContext *context, const Point *pt) {
  if (menu == gameMenu) {
    return GameMenuMouse((GameMenu *)menu, context, pt);
  } else if (menu == mainMenu) {
    return MainMenuMouse((MainMenu *)menu, context, pt);
  }
  return 0;
}

int MenuKeyDown(Menu *menu, GameContext *context, const SDL_Event *e) {
  if (menu == gameMenu) {
    return GameMenuKeyDown((GameMenu *)menu, context, e);
  } else if (menu == mainMenu) {
    return MainMenuKeyDown((MainMenu *)menu, context, e);
  }
  assert(0);
  return 0;
}

/* Main Menu */

static void MainMenuReset(MainMenu *menu) {}

static int MainMenuMouse_MainMenu(MainMenu *menu, GameContext *context,
                                  const Point *pt) {
  const int width = 128;
  const int height = 8;
  if (zoneClicked(pt, 86, 144, width, height)) {
    printf("Start a new game\n");
    menu->state = MainMenuState_StartNew;
    return 1;
  } else if (zoneClicked(pt, 86, 153, width, height)) {
    printf("Introduction\n");
    menu->state = MainMenuState_Introduction;
    return 1;
  } else if (zoneClicked(pt, 86, 162, width, height)) {
    printf("Lore of the lands\n");
    menu->state = MainMenuState_LoreOfTheLands;
    return 1;
  } else if (zoneClicked(pt, 86, 171, width, height)) {
    printf("Load a game\n");
    menu->state = MainMenuState_LoadGame;
    return 1;
  } else if (zoneClicked(pt, 86, 180, width, height)) {
    printf("Exit\n");
    GameContextExitGame(context);
  }
  return 0;
}

static int MainMenuMouse_UnimplementedMenu(MainMenu *menu, GameContext *context,
                                           const Point *pt) {
  int startX = 16;
  int startY = 140;
  if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    menu->state = MainMenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int MainMenuMouse(MainMenu *menu, GameContext *context,
                         const Point *pt) {
  switch (menu->state) {

  case MainMenuState_GameMenu:
    return MainMenuMouse_MainMenu(menu, context, pt);
  case MainMenuState_StartNew:
  case MainMenuState_Introduction:
  case MainMenuState_LoreOfTheLands:
  case MainMenuState_LoadGame:
    return MainMenuMouse_UnimplementedMenu(menu, context, pt);
  case MainMenuState_Exit:
    break;
  }
  return 0;
}

static int MainMenuKeyDown_UnimplementedMenu(MainMenu *menu,
                                             GameContext *context,
                                             const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_RETURN:
    menu->state = MainMenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int MainMenuKeyDown_GameMenu(MainMenu *menu, GameContext *context,
                                    const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_UP:
  case SDLK_DOWN:
    menu->base.selectedIndex += e->key.keysym.sym == SDLK_UP ? -1 : 1;
    if (menu->base.selectedIndex < 0) {
      menu->base.selectedIndex = 5 + menu->base.selectedIndex;
    } else if (menu->base.selectedIndex > 4) {
      menu->base.selectedIndex %= 5;
    }
    return 1;
  case SDLK_RETURN:

    menu->state = menu->base.selectedIndex + 1;
    return 1;
  }
  return 0;
}

static int MainMenuKeyDown(MainMenu *menu, GameContext *context,
                           const SDL_Event *e) {
  switch (menu->state) {

  case MainMenuState_GameMenu:
    return MainMenuKeyDown_GameMenu(menu, context, e);
  case MainMenuState_StartNew:
  case MainMenuState_Introduction:
  case MainMenuState_LoreOfTheLands:
  case MainMenuState_LoadGame:
    return MainMenuKeyDown_UnimplementedMenu(menu, context, e);
  case MainMenuState_Exit:
    break;
  }
  return 0;
}

static void MainMenuRender_UnimplementedMenu(MainMenu *menu,
                                             GameContext *context,
                                             const FNTHandle *font,
                                             SDL_Texture *pixBuf) {
  int startX = 16;
  int startY = 140;
  int width = 288;
  int height = 52;
  UIDrawMenuWindow(pixBuf, startX, startY, width, height);

  GameContextGetString(context, 0X400A, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, startX + width / 2, startY + 8,
                       "unimplemented feature");

  GameContextGetString(context, STR_NO_INDEX, textBuf, 128);
  UISetTextStyle(UITextStyle_Highlighted);
  UIDrawButton(font, pixBuf, startX + 208, startY + 30, 72, 15, "ok");
  UIResetTextStyle();
}

static void MainMenuRender_MainMenu(MainMenu *menu, GameContext *context,
                                    const FNTHandle *font,
                                    SDL_Texture *pixBuf) {
  renderCPS(context->pixBuf, context->gameTitle.data,
            context->gameTitle.imageSize, context->gameTitle.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);

  UISetStyle(UIStyle_MainMenu);
  UIDrawMenuWindow(context->pixBuf, 86, 140, 128, 51);

  const int xCenter = 86 + (128 / 2);

  GameContextGetString(context, 0X4248, textBuf, 128);
  if (menu->base.selectedIndex == 0) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 144,
                       textBuf);
  UIResetTextStyle();

  if (menu->base.selectedIndex == 1) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X4249, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 153,
                       textBuf);
  UIResetTextStyle();

  if (menu->base.selectedIndex == 2) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X42DD, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 162,
                       textBuf);
  UIResetTextStyle();

  if (menu->base.selectedIndex == 3) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X4001, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 171,
                       textBuf);
  UIResetTextStyle();

  if (menu->base.selectedIndex == 4) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X424A, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 180,
                       textBuf);
  UIResetTextStyle();
}

static void MainMenuRender(MainMenu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf) {
  switch (menu->state) {
  case MainMenuState_GameMenu:
    MainMenuRender_MainMenu(menu, context, font, pixBuf);
    break;
  case MainMenuState_StartNew:
    printf("Start new game\n");
    GameContextNewGame(context);
    menu->base.returnToGame = 1;
    break;
  case MainMenuState_Introduction:
  case MainMenuState_LoreOfTheLands:
  case MainMenuState_LoadGame:
    MainMenuRender_UnimplementedMenu(menu, context, font, pixBuf);
    break;
  case MainMenuState_Exit:
    GameContextExitGame(context);
    break;
  }
}

/* Game Menu*/

static void GameMenuReset(GameMenu *menu) {
  menu->state = GameMenuState_GameMenu;
}

static void GameMenuRender_ExitGame(GameMenu *menu, GameContext *context,
                                    const FNTHandle *font,
                                    SDL_Texture *pixBuf) {
  int startX = 16;
  int startY = 72;
  int width = 288;
  int height = 52;
  UIDrawMenuWindow(pixBuf, startX, startY, width, height);

  GameContextGetString(context, 0X400A, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, startX + width / 2, startY + 8, textBuf);

  GameContextGetString(context, STR_YES_INDEX, textBuf, 128);
  UIDrawButton(font, pixBuf, startX + 8, startY + 30, 72, 15, textBuf);

  GameContextGetString(context, STR_NO_INDEX, textBuf, 128);
  UIDrawButton(font, pixBuf, startX + 208, startY + 30, 72, 15, textBuf);
}

static void GameMenuRender_MainMenu(GameMenu *menu, GameContext *context,
                                    const FNTHandle *font,
                                    SDL_Texture *pixBuf) {
  UIDrawMenuWindow(pixBuf, GAME_MENU_X, GAME_MENU_Y, GAME_MENU_W, GAME_MENU_H);

  GameContextGetString(context, 0X4000, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, GAME_MENU_X + GAME_MENU_W / 2,
                       GAME_MENU_Y + 10, textBuf);

  int buttonY = GAME_MENU_BUTTONS_START_Y;
  GameContextGetString(context, 0X4001, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4002, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4003, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  // game controls
  GameContextGetString(context, 0X4004, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X42D9, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4006, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
               GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTON_H + 4;
  GameContextGetString(context, 0X4005, textBuf, 128);
  UIDrawButton(font, pixBuf, GAME_MENU_RESUME_BUTTON_X, GAME_MENU_Y + buttonY,
               GAME_MENU_RESUME_BUTTON_W, GAME_MENU_BUTTON_H, textBuf);
}

static void GameMenuRender(GameMenu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf) {
  UISetStyle(UIStyle_GameMenu);
  switch (menu->state) {

  case GameMenuState_GameMenu:
    GameMenuRender_MainMenu(menu, context, font, pixBuf);
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
    GameMenuRender_ExitGame(menu, context, font, pixBuf);
    break;
  }
}

static int GameMenuMouse_ExitGame(GameMenu *menu, GameContext *context,
                                  const Point *pt) {

  int startX = 16;
  int startY = 72;
  if (zoneClicked(pt, startX + 8, startY + 30, 72, 15)) {
    GameContextExitGame(context);
    return 1;
  } else if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    menu->state = GameMenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int GameMenuMouse_MainMenu(GameMenu *menu, GameContext *context,
                                  const Point *pt) {
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
    menu->base.returnToGame = 1;
    printf("resume\n");
    return 1;
  }

  return 0;
}

static int GameMenuMouse(GameMenu *menu, GameContext *context,
                         const Point *pt) {
  switch (menu->state) {

  case GameMenuState_GameMenu:
    return GameMenuMouse_MainMenu(menu, context, pt);
  case GameMenuState_LoadGame:
  case GameMenuState_SaveGame:
  case GameMenuState_DeleteGame:
  case GameMenuState_GameControls:
  case GameMenuState_AudioControls:
  case GameMenuState_ExitGame:
    return GameMenuMouse_ExitGame(menu, context, pt);
    break;
  }
  return 0;
}

static int GameMenuKeyDown(GameMenu *menu, GameContext *context,
                           const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_ESCAPE:
    if (menu->state == GameMenuState_GameMenu) {
      menu->base.returnToGame = 1;
    } else {
      menu->state = GameMenuState_GameMenu;
    }
    return 1;
  }
  return 0;
}
