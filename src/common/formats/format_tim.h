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

typedef struct {
  uint8_t *text;
  size_t textSize;

  int numTextStrings;

  uint16_t *avtl;
  size_t avtlSize; // size in sizeof(uint16_t)!
} TIMHandle;

void TIMHandleInit(TIMHandle *handle);

int TIMHandleFromBuffer(TIMHandle *handle, const uint8_t *buffer,
                        size_t bufferSize);

const char *TIMHandleGetText(const TIMHandle *handle, int index);
