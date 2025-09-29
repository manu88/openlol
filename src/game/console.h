#pragma once
#include <SDL.h>
typedef struct _GameContext GameContext;

void setupConsole(GameContext *gameCtx);
int processConsoleInputs(GameContext *gameCtx, const SDL_Event *e);
