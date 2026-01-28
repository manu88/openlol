#pragma once
#include <stddef.h>
#include <stdint.h>

static inline uint16_t swap_uint16(uint16_t val) {
  return (val << 8) | (val >> 8);
}

static inline uint32_t swap_uint32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}

uint8_t *readBinaryFile(const char *path, size_t *fileSize, size_t *readSize);
uint8_t *readBinaryFilePart(const char *path, size_t sizeToRead);

int writeBinaryFile(const char *path, void *data, size_t dataSize);
