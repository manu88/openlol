#pragma once

#include "formats/format_fnt.h"
#include "geometry.h"
#include <SDL2/SDL.h>

typedef struct _GameContext GameContext;

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
  int returnToGame;
} Menu;

typedef struct {
  Menu base;
  GameMenuState state;
} GameMenu;

extern Menu *gameMenu;
extern Menu *mainMenu;

void MenuReset(Menu *menu);
void MenuRender(Menu *menu, GameContext *context, const FNTHandle *font,
                SDL_Texture *texture);

int MenuMouse(Menu *menu, GameContext *context, const Point *pt);
int MenuKeyDown(Menu *menu, GameContext *context, const SDL_Event *e);
