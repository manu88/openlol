#pragma once
#include "game_ctx.h"
#include "tim_interpreter.h"
#include <stdint.h>

#define TIM_NUM_ANIMATIONS 4
#define WSA_NUM_ANIMATIONS 6
#define NUM_ANIMATIONS_PARTS 4

typedef struct {
  uint16_t firstFrame;
  uint16_t lastFrame;
  uint16_t cycles;
  uint16_t nextPart;
  uint16_t partDelay;
  uint16_t field;
  uint16_t sfxIndex;
  uint16_t sfxFrame;

  uint16_t currentFrame;
} AnimationPart;

typedef struct {
  WSAHandle wsa;
  int x;
  int y;
  uint8_t loaded;
  AnimationPart parts[NUM_ANIMATIONS_PARTS];
  AnimationPart *currentPart;
} Animation;

typedef struct {
  TIMHandle scripts[TIM_NUM_ANIMATIONS];
  TIMInterpreter interp;
  Animation anims[WSA_NUM_ANIMATIONS];

  uint8_t *frameBuffer;
  size_t frameBufferSize;
} TIMContext;

void TIMLoad(uint16_t scriptId, const char *file);
void TIMRun(GameContext *gameCtx, uint16_t scriptId, uint16_t loop);
void TIMRelease(uint16_t scriptId);

void TimLoadWSA(GameContext *gameCtx, uint16_t index, const char *wsaFile,
                int x, int y, int offscreen, int flags);

void TimSetupPart(GameContext *gameCtx, uint16_t animIndex, uint16_t partIndex,
                  uint16_t firstFrame, uint16_t lastFrame, uint16_t cycles,
                  uint16_t nextPart, uint16_t partDelay, uint16_t field,
                  uint16_t sfxIndex, uint16_t sfxFrame);
void TimStartPart(GameContext *gameCtx, uint16_t animIndex, uint16_t partIndex);
