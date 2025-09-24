#include "tim_animator.h"
#include "tim_interpreter.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

static int initSDLStuff(TIMAnimator *animator) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 0;
  }
  animator->window = SDL_CreateWindow("TIM animator", SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                      SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (animator->window == NULL) {
    return 0;
  }
  return 1;
}

void TIMAnimatorInit(TIMAnimator *animator) {
  memset(animator, 0, sizeof(TIMAnimator));
  TIMInterpreterInit(&animator->_interpreter);
}
void TIMAnimatorRelease(TIMAnimator *animator) {
  TIMInterpreterRelease(&animator->_interpreter);
  if (animator->window) {
    SDL_DestroyWindow(animator->window);
  }
}

static void mainLoop(TIMAnimator *animator) {
  SDL_Surface *screenSurface = screenSurface =
      SDL_GetWindowSurface(animator->window);
  SDL_FillRect(screenSurface, NULL,
               SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
  SDL_UpdateWindowSurface(animator->window);

  int quit = 0;
  while (!quit) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = 1;
    }
  }
}

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim) {
  if (initSDLStuff(animator) == 0) {
    return 0;
  }
  printf("TIMAnimatorRunAnim\n");
  animator->tim = tim;
  mainLoop(animator);
  return TIMInterpreterExec(&animator->_interpreter, animator->tim);
}
