#pragma once

#include "formats/format_wsa.h"
#include <SDL2/SDL.h>

typedef struct {
  WSAHandle wsa;
  int wsaFlags;
  int wsaX;
  int wsaY;

  uint8_t *wsaFrameBuffer;
  SDL_Texture *pixBuf;

  uint8_t *defaultPalette;

} Animator;

void AnimatorInit(Animator *animator);
void AnimatorRelease(Animator *animator);

void AnimatorInitWSA(Animator *animator, const uint8_t *buffer,
                     size_t bufferSize, int x, int y, int offscreen, int flags);
void AnimatorRenderWSAFrame(Animator *animator);

void AnimatorSetupPart(Animator *animator, uint16_t animIndex, uint16_t part,
                       uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles,
                       uint16_t nextPart, uint16_t partDelay, uint16_t field,
                       uint16_t sfxIndex, uint16_t sfxFrame);

void AnimatorPlayPart(Animator *animator, uint16_t animIndex,
                      uint16_t firstFrame, uint16_t lastFrame, uint16_t delay);
