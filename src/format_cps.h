#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
  CPSCompressionType_Uncompressed = 0,
  CPSCompressionType_LZW_12 = 1,
  CPSCompressionType_LZW_14 = 2,
  CPSCompressionType_LZW_RLE = 3,
  CPSCompressionType_LZW_LCW = 4,
} CPSCompressionType;

#define PALETTE_SIZE_256_6_RGB_VGA 768

typedef struct {
  uint16_t fileSize; // For compression method 0 and 4, this only counts the
                     // bytes behind this value, making it two bytes less than
                     // the actual file size.
  uint16_t compressionType;  // see CPSCompressionType
  uint32_t uncompressedSize; // For PC files, this should always be 64000, since
                             // the data contains a linear 8-bit 320×200 image.
  uint16_t paletteSize; // This can either be 0, for no palette, 768 for a PC
                        // 256-colour 6-bit RGB VGA palette,
} CPSFileHeader;

uint8_t *TestCps(const uint8_t *buffer, size_t bufferSize);
int ParseCPSBuffer(uint8_t *inBuf, uint32_t inLen, uint8_t *outBuf,
                   uint32_t outLen);
