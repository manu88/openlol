#pragma once
#include <stddef.h>
#include <stdint.h>

#define TABLE_1_SIZE 256
#define TABLE_2_SIZE 5120

typedef struct {
  uint8_t *table1;
  uint8_t *table2;
} TLCHandle;

int TLCHandleFromBuffer(TLCHandle *handle, uint8_t *buffer, size_t bufferSize);
