#pragma once

#include "formats/format_fnt.h"
#include "geometry.h"
#include <SDL2/SDL.h>

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
} GameMenu;

extern GameMenu *gameMenu;
extern GameMenu *mainMenu;

void GameMenuReset(GameMenu *menu);
void GameMenuRender(GameMenu *menu, const FNTHandle *font,
                    SDL_Texture *texture);

int GameMenuMouse(GameMenu* menu, const Point *pt);

