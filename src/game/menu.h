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
  GameMenuState state;
  int returnToGame;
} GameMenu;

extern GameMenu *gameMenu;
extern GameMenu *mainMenu;

void GameMenuReset(GameMenu *menu);
void GameMenuRender(GameMenu *menu,GameContext *context, const FNTHandle *font,
                    SDL_Texture *texture);

int GameMenuMouse(GameMenu *menu, GameContext *context, const Point *pt);
int GameMenuKeyDown(GameMenu *menu, GameContext *context, const SDL_Event *e);
