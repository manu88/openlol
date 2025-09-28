#pragma once
#include "SDL_render.h"
#include "format_lang.h"
#include "format_tim.h"
#include "format_wsa.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

typedef struct {
  WSAHandle wsa;
  int animXOffset;
  int animYOffset;
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

  uint8_t buttonsState[3];

  uint8_t *wsaFrameBuffer;
  int wsaFlags;
} TIMAnimator;

void TIMAnimatorInit(TIMAnimator *animator);
void TIMAnimatorRelease(TIMAnimator *animator);

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim);
