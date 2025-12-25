#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  char signature[19];
  uint8_t sig; // 1A
  uint16_t headerSize;
  uint16_t version;
  uint16_t checkSum;
} VOCHeader;

typedef enum __attribute__((__packed__)) {
  VOCBlockType_Terminator = 0,
  VOCBlockType_SoundDataTyped = 1,
  VOCBlockType_SoundDataUntyped = 2,
  VOCBlockType_Silence = 3,
  VOCBlockType_Marker = 4,
  VOCBlockType_Text = 5,
  VOCBlockType_RepeatStart = 6,
  VOCBlockType_RepeatEnd = 7,
  VOCBlockType_ExtraInfo = 8,
  VOCBlockType_SoundData2 = 9,
} VOCBlockType;

static_assert(sizeof(VOCBlockType) == 1, "");

typedef enum __attribute__((__packed__)) {
  VOCCodec_PCM_U8 = 0,
} VOCCodec;

static_assert(sizeof(VOCCodec) == 1, "");

typedef struct {
  uint8_t freqDivisor;
  VOCCodec codec;
  uint8_t *data;
} VOCSoundDataTyped;

typedef struct __attribute__((__packed__)) {
  VOCBlockType type;
  uint8_t
      size[3]; // Block size, excluding this four-byte header - 24 bits value
} VOCBlock;

static_assert(sizeof(VOCBlock) == 4, "");

static inline uint32_t VOCBlockGetSize(const VOCBlock *block) {
  return (block->size[2] << 16) + (block->size[1] << 8) + block->size[0];
}

int VOCBlockIsLast(const VOCBlock *block);

const uint8_t *VOCBlockGetData(const VOCBlock *block);

typedef struct {

  VOCHeader *header;
  const VOCBlock *firstBlock;
} VOCHandle;

int VOCHandleFromBuffer(VOCHandle *handle, const uint8_t *data,
                        size_t dataSize);

const VOCBlock *VOCHandleGetNextBlock(const VOCHandle *handle,
                                      const VOCBlock *block);
