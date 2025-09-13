#pragma once
#include "game.h"
#include <SDL2/SDL.h>

void GameRenderFrame(SDL_Renderer *renderer, LevelContext *ctx);
void GameRenderView(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                    int yOff);

void GameRenderMap(SDL_Renderer *renderer, LevelContext *ctx, int xOff,
                   int yOff);
