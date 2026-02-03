#pragma once

#include "formats/format_fnt.h"
#include "geometry.h"
#include <SDL2/SDL.h>
#include <stddef.h>

typedef struct _GameContext GameContext;
typedef struct _SAVFile SAVFile;

typedef enum {
  MenuState_GameMenu,
  MenuState_LoadGame,
  MenuState_SaveGame,
  MenuState_DeleteGame,
  MenuState_GameControls,
  MenuState_AudioControls,
  MenuState_ExitGame,
  MenuState_StartNew,
  MenuState_Introduction,
  MenuState_LoreOfTheLands,
} MenuState;

typedef struct {
  MenuState state;
  int returnToGame;
  int selectedIndex;

  size_t numSavFiles;
  SAVFile *files;
} Menu;

extern Menu *gameMenu;
extern Menu *mainMenu;

void MenuReset(Menu *menu);
void MenuRender(Menu *menu, GameContext *context, const FNTHandle *font);

int MenuMouse(Menu *menu, GameContext *context, const Point *pt);
int MenuKeyDown(Menu *menu, GameContext *context, const SDL_Event *e);
