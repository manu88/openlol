#include "format_shp.h"
#include "format_lcw.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void SHPHandleRelease(SHPHandle *handle) {
  if (handle->toFree) {
    free(handle->toFree);
  }
}

#define SHPBITMAPHEADER_SIZE 10
typedef struct {
  uint16_t fileSize;
  uint16_t compressionType;
  uint32_t uncompressedSize;
  uint16_t paletteSize;
} CompressedSHPHeader;

int SHPHandleFromCompressedBuffer(SHPHandle *handle, uint8_t *buffer,
                                  size_t size) {
  const CompressedSHPHeader *header = (const CompressedSHPHeader *)buffer;
  handle->toFree = malloc(header->uncompressedSize);
  assert(header->paletteSize == 0);
  if (!handle->toFree) {
    return 0;
  }
  assert(LCWDecompress(buffer + SHPBITMAPHEADER_SIZE,
                       size - SHPBITMAPHEADER_SIZE, handle->toFree,
                       header->uncompressedSize) == header->uncompressedSize);
  return SHPHandleFromBuffer(handle, handle->toFree, header->uncompressedSize);
}

int SHPHandleFromBuffer(SHPHandle *handle, uint8_t *buffer, size_t size) {
  handle->originalBuffer = buffer;

  handle->framesCount = (uint16_t)*(handle->originalBuffer);
  handle->frameOffsets = (uint32_t *)(handle->originalBuffer + 2);
  return 1;
}

uint32_t SHPHandleGetFrame(const SHPHandle *handle, SHPFrame *frame,
                           size_t index) {
  if (index >= handle->framesCount) {
    printf("SHPHandleGetFrame index %zu exceeds num frames %i\n", index,
           handle->framesCount);
  }
  assert(index < handle->framesCount);
  uint32_t offset = handle->frameOffsets[index];
  uint8_t *frameStart = handle->originalBuffer + 2 + offset;

  frame->header.flags[0] = *frameStart;
  frame->header.flags[1] = *frameStart + 1;
  frame->header.slices = *frameStart + 2;
  frame->header.width = *(uint16_t *)(frameStart + 3);
  frame->header.height = *(frameStart + 5);
  frame->header.fileSize = *(uint16_t *)(frameStart + 6);
  frame->header.zeroCompressedSize = *(uint16_t *)(frameStart + 8);

  int nexOffset = 10;
  if (frame->header.flags[0] & CustomSizeRemap) {
    frame->header.remapSize = *(frameStart + nexOffset);
    nexOffset = 11;
  } else if (frame->header.flags[0] & HasRemapTable) {
    frame->header.remapSize = 16;
  }
  if (frame->header.flags[0] & HasRemapTable) {
    frame->header.remapTable = frameStart + nexOffset;
  }
  frame->undecodedImageData = frameStart + nexOffset + frame->header.remapSize;
  frame->headerSize = nexOffset + frame->header.remapSize;
  return offset;
}

void SHPFrameRelease(SHPFrame *frame) {
  if (frame->imageBuffer) {
    free(frame->imageBuffer);
  }
}

int SHPFrameGetImageData(SHPFrame *frame) {
  if (frame->imageBuffer) {
    return 1;
  }
  uint8_t hasRemapTable = frame->header.flags[0] & HasRemapTable;
  uint8_t noLCW = frame->header.flags[0] & NoLCW;
  uint8_t customSizeRemap = frame->header.flags[0] & CustomSizeRemap;
  if (customSizeRemap) {
    assert(hasRemapTable);
  }

  const size_t compressedSize = frame->header.fileSize - frame->headerSize;
  uint8_t *imageBuffer = NULL;
  uint8_t shouldFreeImageBuffer = 0;
  if (noLCW == 0) {
    imageBuffer = malloc(frame->header.zeroCompressedSize);
    LCWDecompress(frame->undecodedImageData, compressedSize, imageBuffer,
                  frame->header.zeroCompressedSize);
    shouldFreeImageBuffer = 1;
  } else {
    imageBuffer = frame->undecodedImageData;
  }
  uint8_t *imageBuffer2 =
      DecompressRLEZeroD2(imageBuffer, frame->header.zeroCompressedSize,
                          frame->header.width, frame->header.height);
  if (shouldFreeImageBuffer) {
    free(imageBuffer);
  }
  // remap
  if (frame->header.remapTable) {
    for (int i = 0; i < frame->header.width * frame->header.height; i++) {
      uint8_t p = imageBuffer2[i];
      assert(p <= frame->header.remapSize);
      imageBuffer2[i] = frame->header.remapTable[p];
    }
  }
  frame->imageBuffer = imageBuffer2;
  return 1;
}

void SHPFramePrint(const SHPFrame *frame) {
  uint8_t hasRemapTable = frame->header.flags[0] & HasRemapTable;
  uint8_t noLCW = frame->header.flags[0] & NoLCW;
  uint8_t customSizeRemap = frame->header.flags[0] & CustomSizeRemap;
  if (customSizeRemap) {
    assert(hasRemapTable);
  }

  printf("flags0=%02X flags1=%02X slices=%i w=%i h=%i fsize=%i "
         "zeroCompressedSize=%i hasRemapTable=%i noLCW=%i "
         "customSizeRemap=%i remapSize=%i remapTable=%p imagedata=%p "
         "header=%i\n",
         frame->header.flags[0], frame->header.flags[1], frame->header.slices,
         frame->header.width, frame->header.height, frame->header.fileSize,
         frame->header.zeroCompressedSize, hasRemapTable, noLCW,
         customSizeRemap, frame->header.remapSize,
         (void *)frame->header.remapTable, (void *)frame->undecodedImageData,
         frame->headerSize);
}

void SHPHandlePrint(const SHPHandle *handle) {
  for (int i = 0; i < handle->framesCount; i++) {
    SHPFrame frame = {0};
    SHPHandleGetFrame(handle, &frame, i);
    printf("%i: ", i);
    SHPFramePrint(&frame);
  }
}
