#pragma once

#include "formats/format_fnt.h"
#include "geometry.h"
#include <SDL2/SDL.h>
#include <stddef.h>

typedef struct _GameContext GameContext;

typedef struct {
  int returnToGame;
  int selectedIndex;
} Menu;

typedef enum {
  GameMenuState_GameMenu,
  GameMenuState_LoadGame,
  GameMenuState_SaveGame,
  GameMenuState_DeleteGame,
  GameMenuState_GameControls,
  GameMenuState_AudioControls,
  GameMenuState_ExitGame,
} GameMenuState;

typedef struct {
  Menu base;
  GameMenuState state;
} GameMenu;

extern Menu *gameMenu;

typedef enum {
  MainMenuState_GameMenu,
  MainMenuState_StartNew,
  MainMenuState_Introduction,
  MainMenuState_LoreOfTheLands,
  MainMenuState_LoadGame,
  MainMenuState_Exit,
} MainMenuState;

typedef struct {
  Menu base;
  MainMenuState state;
  size_t numSavFiles;
} MainMenu;

extern Menu *mainMenu;

void MenuReset(Menu *menu);
void MenuRender(Menu *menu, GameContext *context, const FNTHandle *font,
                SDL_Texture *texture);

int MenuMouse(Menu *menu, GameContext *context, const Point *pt);
int MenuKeyDown(Menu *menu, GameContext *context, const SDL_Event *e);
