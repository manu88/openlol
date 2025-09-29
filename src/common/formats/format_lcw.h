#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

ssize_t LCWDecompress(const uint8_t *source, size_t sourceSize, uint8_t *dest,
                      size_t destSize);

ssize_t LCWCompress(void const *input, void *output, unsigned long size);

uint8_t *DecompressRLEZeroD2(const uint8_t *fileData, size_t datalen,
                             int frameWidth, int frameHeight);
