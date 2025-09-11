#include "format_shp.h"
#include "format_lcw.h"
#include "renderer.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void SHPHandleRelease(SHPHandle *handle) { free(handle->originalBuffer); }

int SHPHandleFromBuffer(SHPHandle *handle, uint8_t *buffer, size_t size) {
  handle->originalBuffer = buffer;

  handle->framesCount = (uint16_t)*(handle->originalBuffer);
  handle->frameOffsets = (uint32_t *)(handle->originalBuffer + 2);
  return 1;
}

uint32_t SHPHandleGetFrame(const SHPHandle *handle, SHPFrame *frame,
                           size_t index) {
  assert(index <= handle->framesCount);
  uint16_t offset = handle->frameOffsets[index];
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
    printf("Remap\n");
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

  printf("flags %02X %02X %i w=%i h=%i fsize=%i "
         "zeroCompressedSize=%i hasRemapTable=%i noLCW=%i "
         "customSizeRemap=%i remapSize %i remapTable %p image data %p "
         "header=%i\n",
         frame->header.flags[0], frame->header.flags[1], frame->header.slices,
         frame->header.width, frame->header.height, frame->header.fileSize,
         frame->header.zeroCompressedSize, hasRemapTable, noLCW,
         customSizeRemap, frame->header.remapSize,
         (void *)frame->header.remapTable, (void *)frame->undecodedImageData,
         frame->headerSize);
}

void SHPHandlePrint(const SHPHandle *handle) {
  printf("%i frames\n", handle->framesCount);
  for (int i = 0; i < handle->framesCount; i++) {

    SHPFrame frame = {0};
    SHPHandleGetFrame(handle, &frame, i);
    printf("%i: ", i);
    SHPFramePrint(&frame);
  }
}
