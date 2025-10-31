#include "format_lang.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const char *LanguageGetExtension(Language lang) {
  switch (lang) {
  case Language_EN:
    return "ENG";
  case Language_FR:
    return "FRE";
  case Language_GR:
    return "GER";
  }
  assert(0);
}

// from https://github.com/OpenDUNE/OpenDUNE/blob/master/src/string.c#L127
uint16_t decompressAndTranslate(const char *s, char *dest, uint16_t destLen) {
  static const char couples[] = " etainosrlhcdupm" /* 1st char */
                                "tasio wb"         /* <SPACE>? */
                                " rnsdalm"         /* e? */
                                "h ieoras"         /* t? */
                                "nrtlc sy"         /* a? */
                                "nstcloer"         /* i? */
                                " dtgesio"         /* n? */
                                "nr ufmsw"         /* o? */
                                " tep.ica"         /* s? */
                                "e oiadur"         /* r? */
                                " laeiyod"         /* l? */
                                "eia otru"         /* h? */
                                "etoakhlr"         /* c? */
                                " eiu,.oa"         /* d? */
                                "nsrctlai"         /* u? */
                                "leoiratp"         /* p? */
                                "eaoip bm";        /* m? */
  uint16_t count;

  for (count = 0; *s != '\0'; s++) {
    uint8_t c = *s;
    if ((c & 0x80) != 0) {
      c &= 0x7F;
      assert(count + 1 < destLen);
      dest[count++] = couples[c >> 3]; /* 1st char */
      c = couples[c + 16];             /* 2nd char */
    } else if (c == 0x1B) {
      c = 0x7F + *(++s);
    }
    assert(count + 1 < destLen);
    dest[count++] = c;
    if (count > destLen - 1) {
      printf(
          "decompressAndTranslate : truncating output ! count=%i destLen=%i\n",
          count, destLen);
      break;
    }
  }
  assert(count < destLen);
  dest[count] = 0;
  return count;
}

uint16_t LangHandleGetString(const LangHandle *handle, uint16_t index,
                             char *outBuffer, size_t outBufferSize) {
  assert(outBuffer);
  assert(index < handle->count);
  uint16_t *offsets = (uint16_t *)handle->originalBuffer;
  const uint16_t off = offsets[index];
  const char *dat = (const char *)handle->originalBuffer + off;
  uint16_t size = decompressAndTranslate(dat, outBuffer, outBufferSize);
  return size;
}

void LangHandleShow(LangHandle *handle) {
  char dest[1024];
  uint16_t *offsets = (uint16_t *)handle->originalBuffer;
  printf("Count = %i\n", handle->count);
  for (int i = 0; i < handle->count; i++) {

    uint16_t size = LangHandleGetString(handle, i, dest, 1024);
    printf("  %i/%i offset=%i size=%i '%s'\n", i, handle->count, offsets[i],
           size, dest);
  }
}

void LangHandleRelease(LangHandle *handle) {}

int LangHandleFromBuffer(LangHandle *handle, uint8_t *buffer,
                         size_t bufferSize) {
  // if the last char is 0x1A then the .eng file is a plain, normal .txt file.
  //  Otherwise, it have encoded text and an header at top:
  assert(buffer[bufferSize - 1] != 0X1A);
  handle->originalBuffer = buffer;
  const uint16_t *offsets = (uint16_t *)handle->originalBuffer;
  handle->count = offsets[0] / 2 - 1;
  return 1;
}

int LangGetString(uint16_t id, uint8_t *useLevelFile) {
  if (id == 0xFFFF)
    return -1;
  assert(useLevelFile);
  uint16_t realId = id & 0x3FFF;
  *useLevelFile = 0;
  if (id & 0x4000) {
    *useLevelFile = 0;
  } else {
    *useLevelFile = 1;
  }
  return realId;
}
