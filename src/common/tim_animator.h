#pragma once
#include "SDL_render.h"
#include "format_lang.h"
#include "format_tim.h"
#include "format_wsa.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
  WSAHandle wsa;
  TIMHandle *tim;
  TIMInterpreter _interpreter;
  LangHandle *lang;

  char *currentDialog;
  char *buttonText[3];
  SDL_Surface *textSurface;
  SDL_Texture *textTexture;
  TTF_Font *font;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *pixBuf;
} TIMAnimator;

void TIMAnimatorInit(TIMAnimator *animator);
void TIMAnimatorRelease(TIMAnimator *animator);

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim);
