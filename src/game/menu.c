#include "menu.h"
#include "SDL_keycode.h"
#include "formats/format_lang.h"
#include "game_ctx.h"
#include "geometry.h"
#include "render.h"
#include "ui.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

static Menu _gameMenu = {0};
static Menu _mainMenu = {0};

Menu *gameMenu = &_gameMenu;
static void GameMenuReset(Menu *menu);
static void GameMenuRender(Menu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf);
static int GameMenuMouse(Menu *menu, GameContext *context, const Point *pt);
static int GameMenuKeyDown(Menu *menu, GameContext *context,
                           const SDL_Event *e);

Menu *mainMenu = &_mainMenu;
static void MainMenuReset(Menu *menu);
static void MainMenuRender(Menu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf);
static int MainMenuMouse(Menu *menu, GameContext *context, const Point *pt);
static int MainMenuKeyDown(Menu *menu, GameContext *context,
                           const SDL_Event *e);
void MainMenuSetState(Menu *menu, MenuState state) {
  menu->state = state;
  menu->selectedIndex = 0;
  if (menu->state != MenuState_LoadGame) {
    SAVFilesRelease(menu->files, menu->numSavFiles);
    menu->files = NULL;
    menu->numSavFiles = 0;
  }
}

static char textBuf[128] = "";

void MenuReset(Menu *menu) {
  menu->returnToGame = 0;
  menu->selectedIndex = 0;
  if (menu == gameMenu) {
    GameMenuReset(menu);
  } else if (menu == mainMenu) {
    MainMenuReset(menu);
  } else {
    assert(0);
  }
}

void MenuRender(Menu *menu, GameContext *context, const FNTHandle *font,
                SDL_Texture *pixBuf) {
  assert(menu);
  if (menu == gameMenu) {
    GameMenuRender(menu, context, font, pixBuf);
  } else if (menu == mainMenu) {
    MainMenuRender(menu, context, font, pixBuf);
  } else {
    assert(0);
  }
}

int MenuMouse(Menu *menu, GameContext *context, const Point *pt) {
  if (menu == gameMenu) {
    return GameMenuMouse(menu, context, pt);
  } else if (menu == mainMenu) {
    return MainMenuMouse(menu, context, pt);
  }
  return 0;
}

int MenuKeyDown(Menu *menu, GameContext *context, const SDL_Event *e) {
  if (menu == gameMenu) {
    return GameMenuKeyDown(menu, context, e);
  } else if (menu == mainMenu) {
    return MainMenuKeyDown(menu, context, e);
  }
  assert(0);
  return 0;
}

/* Main Menu */

static void MainMenuReset(Menu *menu) { menu->numSavFiles = 0; }

static int MainMenuMouse_MainMenu(Menu *menu, GameContext *context,
                                  const Point *pt) {
  const int width = 128;
  const int height = 8;
  if (zoneClicked(pt, 86, 144, width, height)) {
    printf("Start a new game\n");
    MainMenuSetState(menu, MenuState_StartNew);
    return 1;
  } else if (zoneClicked(pt, 86, 153, width, height)) {
    printf("Introduction\n");
    MainMenuSetState(menu, MenuState_Introduction);
    return 1;
  } else if (zoneClicked(pt, 86, 162, width, height)) {
    printf("Lore of the lands\n");
    MainMenuSetState(menu, MenuState_LoreOfTheLands);
    return 1;
  } else if (zoneClicked(pt, 86, 171, width, height)) {
    printf("Load a game\n");
    MainMenuSetState(menu, MenuState_LoadGame);
    return 1;
  } else if (zoneClicked(pt, 86, 180, width, height)) {
    printf("Exit\n");
    GameContextExitGame(context);
  }
  return 0;
}

static int MenuMouse_LoadMenu(Menu *menu, GameContext *context,
                              const Point *pt) {
  const int winX = 23;
  const int winY = 30;

  if (menu->selectedIndex > 0 && zoneClicked(pt, 150, 50, 24, 15)) {
    menu->selectedIndex--;
    if (menu->selectedIndex < 0) {
      menu->selectedIndex = 0;
    }
    return 1;
  }

  if (menu->selectedIndex < menu->numSavFiles - 4 &&
      zoneClicked(pt, 150, 148, 24, 15)) {
    if (menu->selectedIndex < menu->numSavFiles - 4) {
      menu->selectedIndex++;
    }
    return 1;
  }

  int maxButtons = menu->numSavFiles < 4 ? menu->numSavFiles : 4;
  for (int i = 0; i < maxButtons; i++) {
    if (zoneClicked(pt, winX + 8, winY + 39 + (i * 17), 256, 15)) {
      printf("Clicked slot %i -> %i\n", i, i + menu->selectedIndex);
      if (GameContextLoadSaveFile(
              context, menu->files[i + menu->selectedIndex].fullpath)) {
        GameContextLoadChars(context);
        menu->returnToGame = 1;
      }
      return 1;
    }
  }
  // cancel
  if (zoneClicked(pt, winX + 168, winY + 118, 96, 15)) {
    MainMenuSetState(menu, MenuState_GameMenu);
    return 1;
  }
  return 0;
}

static int MainMenuMouse_UnimplementedMenu(Menu *menu, GameContext *context,
                                           const Point *pt) {
  int startX = 16;
  int startY = 140;
  if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    MainMenuSetState(menu, MenuState_GameMenu);
    return 1;
  }
  return 0;
}

static int MainMenuMouse(Menu *menu, GameContext *context, const Point *pt) {
  switch (menu->state) {
  case MenuState_GameMenu:
    return MainMenuMouse_MainMenu(menu, context, pt);
  case MenuState_LoadGame:
    return MenuMouse_LoadMenu(menu, context, pt);
  case MenuState_StartNew:
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:
    return MainMenuMouse_UnimplementedMenu(menu, context, pt);
  case MenuState_ExitGame:
    break;
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
    assert(0);
  }
  return 0;
}

static int MainMenuKeyDown_UnimplementedMenu(Menu *menu, GameContext *context,
                                             const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_RETURN:
    MainMenuSetState(menu, MenuState_GameMenu);
    return 1;
  }
  return 0;
}

static MenuState MainMenuStates[] = {
    MenuState_StartNew, MenuState_Introduction, MenuState_LoreOfTheLands,
    MenuState_LoadGame, MenuState_ExitGame,
};

static int MainMenuKeyDown_GameMenu(Menu *menu, GameContext *context,
                                    const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_UP:
  case SDLK_DOWN:
    menu->selectedIndex += e->key.keysym.sym == SDLK_UP ? -1 : 1;
    if (menu->selectedIndex < 0) {
      menu->selectedIndex = 5 + menu->selectedIndex;
    } else if (menu->selectedIndex > 4) {
      menu->selectedIndex %= 5;
    }
    return 1;
  case SDLK_RETURN:
    MainMenuSetState(menu, MainMenuStates[menu->selectedIndex]);
    return 1;
  }
  return 0;
}

static int MenuKeyDown_LoadMenu(Menu *menu, GameContext *context,
                                const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_ESCAPE:
    MainMenuSetState(menu, 0);
    return 1;
  case SDLK_UP:
  case SDLK_DOWN:
    menu->selectedIndex += e->key.keysym.sym == SDLK_UP ? -1 : 1;
    if (menu->selectedIndex < 0) {
      menu->selectedIndex = 0;
    } else if (menu->selectedIndex > menu->numSavFiles - 4) {
      menu->selectedIndex = menu->numSavFiles - 4;
    }

    return 1;
  }
  return 0;
}

static int MainMenuKeyDown(Menu *menu, GameContext *context,
                           const SDL_Event *e) {
  switch (menu->state) {
  case MenuState_GameMenu:
    return MainMenuKeyDown_GameMenu(menu, context, e);
  case MenuState_LoadGame:
    return MenuKeyDown_LoadMenu(menu, context, e);
  case MenuState_StartNew:
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:
    return MainMenuKeyDown_UnimplementedMenu(menu, context, e);
  case MenuState_ExitGame:
    break;
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
    assert(0);
  }
  return 0;
}

static void MenuRender_LoadMenu(Menu *menu, GameContext *context,
                                const FNTHandle *font, SDL_Texture *pixBuf) {
  UIStyle current = UIGetCurrentStyle();
  UISetStyle(UIStyle_GameMenu);
  const int winX = 23;
  const int winY = 30;
  const int winW = 272;
  const int winH = 140;
  UIDrawMenuWindow(pixBuf, winX, winY, winW, winH);

  // up arrow
  if (menu->selectedIndex > 0) {
    UIDrawButton(context->pixBuf, 150, 50, 24, 15);
  }

  GameContextGetString(context, 0X400E, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, winX + winW / 2, winY + 8, textBuf);

  if (menu->files == NULL) {
    menu->files = GameContextListSavFiles(context, &menu->numSavFiles);
    if (menu->numSavFiles) {
      assert(menu->files);
    }
  }
  int maxButtons = menu->numSavFiles < 4 ? menu->numSavFiles : 4;

  for (int i = 0; i < maxButtons; i++) {
    if (i + menu->selectedIndex >= menu->numSavFiles) {
      break;
    }
    UIDrawTextButton(font, pixBuf, winX + 8, winY + 39 + (i * 17), 256, 15,
                     menu->files[i + menu->selectedIndex].savName);
  }

  // down arrow
  if (menu->numSavFiles > 0 && menu->selectedIndex < menu->numSavFiles - 4) {
    UIDrawButton(context->pixBuf, 150, 148, 24, 15);
  }

  // cancel
  GameContextGetString(context, 0X4011, textBuf, 128);
  UIDrawTextButton(&context->defaultFont, context->pixBuf, winX + 168,
                   winY + 118, 96, 15, textBuf);
  UISetStyle(current);
}

static void MainMenuRender_UnimplementedMenu(Menu *menu, GameContext *context,
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
  UIDrawTextButton(font, pixBuf, startX + 208, startY + 30, 72, 15, "ok");
  UIResetTextStyle();
}

static void MainMenuRender_MainMenu(Menu *menu, GameContext *context,
                                    const FNTHandle *font,
                                    SDL_Texture *pixBuf) {
  UISetStyle(UIStyle_MainMenu);
  UIDrawMenuWindow(context->pixBuf, 86, 140, 128, 51);

  const int xCenter = 86 + (128 / 2);

  GameContextGetString(context, 0X4248, textBuf, 128);
  if (menu->selectedIndex == 0) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 144,
                       textBuf);
  UIResetTextStyle();

  if (menu->selectedIndex == 1) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X4249, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 153,
                       textBuf);
  UIResetTextStyle();

  if (menu->selectedIndex == 2) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X42DD, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 162,
                       textBuf);
  UIResetTextStyle();

  if (menu->selectedIndex == 3) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X4001, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 171,
                       textBuf);
  UIResetTextStyle();

  if (menu->selectedIndex == 4) {
    UISetTextStyle(UITextStyle_Highlighted);
  }
  GameContextGetString(context, 0X424A, textBuf, 128);
  UIRenderTextCentered(&context->defaultFont, context->pixBuf, xCenter, 180,
                       textBuf);
  UIResetTextStyle();
}

static void MainMenuRender(Menu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf) {
  renderCPS(context->pixBuf, context->gameTitle.data,
            context->gameTitle.imageSize, context->gameTitle.palette,
            PIX_BUF_WIDTH, PIX_BUF_HEIGHT);
  switch (menu->state) {
  case MenuState_GameMenu:
    MainMenuRender_MainMenu(menu, context, font, pixBuf);
    break;
  case MenuState_StartNew:
    GameContextNewGame(context);
    menu->returnToGame = 1;
    break;
  case MenuState_LoadGame:
    MenuRender_LoadMenu(menu, context, font, pixBuf);
    break;
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:

    MainMenuRender_UnimplementedMenu(menu, context, font, pixBuf);
    break;
  case MenuState_ExitGame:
    GameContextExitGame(context);
    break;
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
    assert(0);
  }
}

/* Game Menu*/

static void GameMenuReset(Menu *menu) { menu->state = MenuState_GameMenu; }

static void GameMenuRender_ExitGame(Menu *menu, GameContext *context,
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
  UIDrawTextButton(font, pixBuf, startX + 8, startY + 30, 72, 15, textBuf);

  GameContextGetString(context, STR_NO_INDEX, textBuf, 128);
  UIDrawTextButton(font, pixBuf, startX + 208, startY + 30, 72, 15, textBuf);
}

static void GameMenuRender_MainMenu(Menu *menu, GameContext *context,
                                    const FNTHandle *font,
                                    SDL_Texture *pixBuf) {
  UIDrawMenuWindow(pixBuf, GAME_MENU_X, GAME_MENU_Y, GAME_MENU_W, GAME_MENU_H);

  GameContextGetString(context, 0X4000, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, GAME_MENU_X + GAME_MENU_W / 2,
                       GAME_MENU_Y + 10, textBuf);

  int buttonY = GAME_MENU_BUTTONS_START_Y;
  GameContextGetString(context, 0X4001, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4002, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4003, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  // game controls
  GameContextGetString(context, 0X4004, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X42D9, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  GameContextGetString(context, 0X4006, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_BUTTONS_START_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);

  buttonY += GAME_MENU_BUTTON_H + 4;
  GameContextGetString(context, 0X4005, textBuf, 128);
  UIDrawTextButton(font, pixBuf, GAME_MENU_RESUME_BUTTON_X,
                   GAME_MENU_Y + buttonY, GAME_MENU_RESUME_BUTTON_W,
                   GAME_MENU_BUTTON_H, textBuf);
}

static void GameMenuRender_UnimplementedMenu(Menu *menu, GameContext *context,
                                             const FNTHandle *font,
                                             SDL_Texture *pixBuf) {
  int startX = 16;
  int startY = 30;
  int width = 288;
  int height = 52;
  UIDrawMenuWindow(pixBuf, startX, startY, width, height);

  GameContextGetString(context, 0X400A, textBuf, 128);
  UIRenderTextCentered(font, pixBuf, startX + width / 2, startY + 8,
                       "unimplemented feature");

  GameContextGetString(context, STR_NO_INDEX, textBuf, 128);
  UISetTextStyle(UITextStyle_Highlighted);
  UIDrawTextButton(font, pixBuf, startX + 208, startY + 30, 72, 15, "ok");
  UIResetTextStyle();
}

static void GameMenuRender(Menu *menu, GameContext *context,
                           const FNTHandle *font, SDL_Texture *pixBuf) {
  UISetStyle(UIStyle_GameMenu);
  switch (menu->state) {
  case MenuState_GameMenu:
    GameMenuRender_MainMenu(menu, context, font, pixBuf);
    break;
  case MenuState_ExitGame:
    GameMenuRender_ExitGame(menu, context, font, pixBuf);
    break;
  case MenuState_LoadGame:
    MenuRender_LoadMenu(menu, context, font, pixBuf);
    break;
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
    GameMenuRender_UnimplementedMenu(menu, context, font, pixBuf);
    break;
  case MenuState_StartNew:
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:
    assert(0);
  }
}

static int GameMenuMouse_ExitGame(Menu *menu, GameContext *context,
                                  const Point *pt) {

  int startX = 16;
  int startY = 72;
  if (zoneClicked(pt, startX + 8, startY + 30, 72, 15)) {
    GameContextExitGame(context);
    return 1;
  } else if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    menu->state = MenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int GameMenuMouse_MainMenu(Menu *menu, GameContext *context,
                                  const Point *pt) {
  int buttonY = GAME_MENU_BUTTONS_START_Y;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Load game\n");
    menu->state = MenuState_LoadGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Save this game\n");
    menu->state = MenuState_SaveGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Delete a game\n");
    menu->state = MenuState_DeleteGame;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Game controls\n");
    menu->state = MenuState_GameControls;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Audio controls\n");
    menu->state = MenuState_AudioControls;
    return 1;
  }

  buttonY += GAME_MENU_BUTTONS_Y_OFFSET;
  if (zoneClicked(pt, GAME_MENU_BUTTONS_START_X, GAME_MENU_Y + buttonY,
                  GAME_MENU_BUTTON_W, GAME_MENU_BUTTON_H)) {
    printf("Exit Game\n");
    menu->state = MenuState_ExitGame;
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

static int GameMenuMouse_UnimplementedMenu(Menu *menu, GameContext *context,
                                           const Point *pt) {
  int startX = 16;
  int startY = 30;
  if (zoneClicked(pt, startX + 208, startY + 30, 72, 15)) {
    menu->state = MenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int GameMenuMouse(Menu *menu, GameContext *context, const Point *pt) {
  switch (menu->state) {
  case MenuState_GameMenu:
    return GameMenuMouse_MainMenu(menu, context, pt);
  case MenuState_ExitGame:
    return GameMenuMouse_ExitGame(menu, context, pt);
  case MenuState_LoadGame:
    return MenuMouse_LoadMenu(menu, context, pt);
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
    return GameMenuMouse_UnimplementedMenu(menu, context, pt);
    break;
  case MenuState_StartNew:
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:
    assert(0);
  }
  return 0;
}

static int GameMenuKeyDown_UnimplementedMenu(Menu *menu, GameContext *context,
                                             const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_RETURN:
    menu->state = MenuState_GameMenu;
    return 1;
  }
  return 0;
}

static int GameMenuKeyDown_MainMenu(Menu *menu, GameContext *context,
                                    const SDL_Event *e) {
  switch (e->key.keysym.sym) {
  case SDLK_ESCAPE:
    if (menu->state == MenuState_GameMenu) {
      menu->returnToGame = 1;
    } else {
      menu->state = MenuState_GameMenu;
    }
    return 1;
  }
  return 0;
}

static int GameMenuKeyDown(Menu *menu, GameContext *context,
                           const SDL_Event *e) {

  switch (menu->state) {
  case MenuState_GameMenu:
    return GameMenuKeyDown_MainMenu(menu, context, e);
  case MenuState_LoadGame:
    return MenuKeyDown_LoadMenu(menu, context, e);
  case MenuState_SaveGame:
  case MenuState_DeleteGame:
  case MenuState_GameControls:
  case MenuState_AudioControls:
  case MenuState_ExitGame:
    return GameMenuKeyDown_UnimplementedMenu(menu, context, e);
  // Those are part of the Main menu
  case MenuState_StartNew:
  case MenuState_Introduction:
  case MenuState_LoreOfTheLands:
    assert(0);
  }
  return 0;
}
