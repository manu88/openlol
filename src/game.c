#include "game.h"
#include "SDL_render.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

const float topLeftX = 100;
const float topLeftY = 50;

const float width = 800;
const float height = 600;

const float centerX = topLeftX + (width / 2);
const float centerY = topLeftY + (height / 2);

const float bottomLeftX = topLeftX;
const float bottomLeftY = topLeftY + height;

const float topRightX = topLeftX + width;
const float topRightY = topLeftY;

const float bottomRightX = topLeftX + width;
const float bottomRightY = topLeftY + height;

const float p0X = topLeftX + (width / 4);
const float p0Y = topLeftY + (height / 4);

const float p1X = bottomLeftX + (width / 4);
const float p1Y = bottomLeftY - (height / 4);

const float p2X = bottomRightX - (width / 4);
const float p2Y = bottomRightY - (height / 4);

const float p3X = topRightX - (width / 4);
const float p3Y = topRightY + (height / 4);

const SDL_Color whiteColor = {255, 255, 255, 0xFF};

SDL_Vertex frontWallVertices[4] = {
    {{p0X, p0Y}, whiteColor, {0.f, 0.f}},
    {{p1X, p1Y}, whiteColor, {0.f, 1.f}},
    {{p2X, p2Y}, whiteColor, {1.f, 1.f}},
    {{p3X, p3Y}, whiteColor, {1.f, 0.f}},

};

SDL_Vertex leftWallVertices[4] = {
    {{topLeftX, topLeftY}, whiteColor, {0.f, 0.f}},
    {{bottomLeftX, bottomLeftY}, whiteColor, {0.f, 1.f}},
    {{p1X, p1Y}, whiteColor, {1.f, 1.f}},
    {{p0X, p0Y}, whiteColor, {1.f, 0.f}},
};

SDL_Vertex rightWallVertices[4] = {
    {{p3X, p3Y}, whiteColor, {0.f, 0.f}},
    {{p2X, p2Y}, whiteColor, {0.f, 1.f}},
    {{bottomRightX, bottomRightY}, whiteColor, {1.f, 1.f}},
    {{topRightX, topRightY}, whiteColor, {1.f, 0.f}},
};

/*
topL.       topR
    p0   p3
       c
    p1   p2
botL.       botR
*/

SDL_Vertex ceilingVertices[4] = {
    {{topLeftX, topLeftY}, whiteColor, {0.f, 0.f}},
    {{p0X, p0Y}, whiteColor, {0.f, 1.f}},
    {{p3X, p3Y}, whiteColor, {1.f, 1.f}},
    {{topRightX, topRightY}, whiteColor, {1.f, 0.f}},
};
const int ceilingIndices[] = {0, 1, 2, 2, 3, 0};

SDL_Vertex floorVertices[4] = {
    {{p1X, p1Y}, whiteColor, {0.f, 0.f}},
    {{bottomLeftX, bottomLeftY}, whiteColor, {0.f, 1.f}},
    {{bottomRightX, bottomRightY}, whiteColor, {1.f, 1.f}},
    {{p2X, p2Y}, whiteColor, {1.f, 0.f}},
};
const int floorIndices[] = {0, 1, 2, 2, 3, 0};

int GameRun(int argc, char *argv[]) {
  printf("Run game\n");

  int width = 1280, height = 720;
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Event e;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  window = SDL_CreateWindow("SDL Chaste Triangle", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, width, height, 0);
  if (window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  SDL_Texture *wallTexture =
      IMG_LoadTexture(renderer, "/Users/manueldeneu/Downloads/Levels3D/"
                                "01_Gladstone/KEEP.VCN_pal_W1_D54.png");

  SDL_Texture *floorTexture = IMG_LoadTexture(
      renderer,
      "/Users/manueldeneu/Downloads/Levels3D/01_Gladstone/L01_floor.png");

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  if (SDL_RenderGeometry(renderer, wallTexture, leftWallVertices, 4,
                         ceilingIndices, 6) < 0) {
    SDL_Log("%s\n", SDL_GetError());
  }

  if (SDL_RenderGeometry(renderer, wallTexture, rightWallVertices, 4,
                         ceilingIndices, 6) < 0) {
    SDL_Log("%s\n", SDL_GetError());
  }

  if (SDL_RenderGeometry(renderer, floorTexture, floorVertices, 4,
                         ceilingIndices, 6) < 0) {
    SDL_Log("%s\n", SDL_GetError());
  }

  if (SDL_RenderGeometry(renderer, floorTexture, ceilingVertices, 4,
                         floorIndices, 6) < 0) {
    SDL_Log("%s\n", SDL_GetError());
  }

  if (SDL_RenderGeometry(renderer, wallTexture, frontWallVertices, 4,
                         floorIndices, 6) < 0) {
    SDL_Log("%s\n", SDL_GetError());
  }

  SDL_RenderPresent(renderer);

  while (e.type != SDL_QUIT) /*wait until any key is pressed and then
released*/
  {
    SDL_PollEvent(&e);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
  return 0;
}
