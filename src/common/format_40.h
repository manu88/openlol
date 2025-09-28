#pragma once
#include <stdint.h>
#include <sys/types.h>

void Format40Decode(const uint8_t *src, size_t srcSize, uint8_t *dst,
                    uint8_t xor);
