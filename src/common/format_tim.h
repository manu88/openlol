#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t *ip;
  uint32_t lastTime;
  uint32_t nextTime;
  uint16_t *loopIp;
  uint16_t *avtl;
} TimFunction;

#define NUM_TIM_FUNCTIONS 10

typedef struct {
  uint8_t *text;
  size_t textSize;

  uint16_t *avtl;
  size_t avtlSize;

  TimFunction functions[NUM_TIM_FUNCTIONS];
  int numFunctions;

  uint8_t *originalBuffer;
} TIMHandle;

void TIMHandleInit(TIMHandle *handle);
void TIMHandleRelease(TIMHandle *handle);
int TIMHandleFromBuffer(TIMHandle *handle, const uint8_t *buffer,
                        size_t bufferSize);

const char *TIMHandleGetText(TIMHandle *handle);
int TIMHandleExec(TIMHandle *handle);
