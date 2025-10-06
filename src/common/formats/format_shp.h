#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// warning, this code only works for SHP v1.07.

typedef enum {
  HasRemapTable = 1 << 0,
  NoLCW = 1 << 1,
  CustomSizeRemap = 1 << 2, // implies HasRemapTable to be set
} SHPFrameFlags;

typedef struct {
  uint8_t flags[2];
  uint8_t slices;
  uint16_t width;
  uint8_t height;
  uint16_t fileSize; // including this header
  uint16_t zeroCompressedSize;

  // This byte is only added if the CustomSizeRemap flag is enabled. If not,
  // RemapSize defaults to 16.
  uint8_t remapSize;

  // This table is only added if the HasRemapTable flag is enabled.
  uint8_t *remapTable; // Array size is remapSize
} SHPFrameHeader;

typedef struct {
  SHPFrameHeader header;
  uint8_t headerSize;

  uint8_t *undecodedImageData; // ref to originalBuffer
  uint8_t *imageBuffer;        // the actual image data, needs to be freed!
} SHPFrame;

typedef struct {
  uint16_t framesCount;

  uint32_t *frameOffsets; // array size = framesCount

  uint8_t *originalBuffer;

  uint8_t *toFree;
} SHPHandle;

void SHPHandleRelease(SHPHandle *handle);
int SHPHandleFromBuffer(SHPHandle *handle, uint8_t *buffer, size_t size);
int SHPHandleFromCompressedBuffer(SHPHandle *handle, uint8_t *buffer,
                                  size_t size);

uint32_t SHPHandleGetFrame(const SHPHandle *handle, SHPFrame *frame,
                           size_t index);

void SHPHandlePrint(const SHPHandle *handle);

void SHPFramePrint(const SHPFrame *frame);
void SHPFrameRelease(SHPFrame *frame);
int SHPFrameGetImageData(SHPFrame *frame);
