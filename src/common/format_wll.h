#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint16_t wallId;
} SomeData;

void TestWLL(const uint8_t *buffer, size_t size);
