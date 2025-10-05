#pragma once

#include "SDL_render.h"
#include "formats/format_tim.h"
#include "formats/format_wsa.h"
#include "tim_interpreter.h"
#include <SDL2/SDL.h>
#include <stdint.h>

#define NUM_TIM_ANIMATIONS 4

typedef struct {

  TIMInterpreter timInterpreter;
  uint16_t currentTimScriptId;

  WSAHandle wsa;
  int wsaFlags;
  int wsaX;
  int wsaY;

  TIMHandle tim[NUM_TIM_ANIMATIONS];

  uint8_t *wsaFrameBuffer;
  SDL_Texture *pixBuf;

  uint8_t *defaultPalette;
} GameTimAnimator;

typedef struct _GameContext GameContext;

void GameTimAnimatorInit(GameContext *gameCtx, SDL_Texture *pixBuf);
void GameTimAnimatorRelease(GameTimAnimator *animator);
void GameTimAnimatorLoadTim(GameTimAnimator *animator, uint16_t scriptId,
                            const char *file);
void GameTimAnimatorRunTim(GameTimAnimator *animator, uint16_t scriptId);
void GameTimAnimatorReleaseTim(GameTimAnimator *animator, uint16_t scriptId);

int GameTimAnimatorRender(GameTimAnimator *animator);
