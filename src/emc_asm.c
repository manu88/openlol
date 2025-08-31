#include "emc_asm.h"
#include <_string.h>
#include <stdint.h>
#define _GNU_SOURCE
#include "script.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *ltrim(char *s) {
  while (isspace(*s))
    s++;
  return s;
}

static char *rtrim(char *s) {
  char *back = s + strlen(s);
  while (isspace(*--back))
    ;
  *(back + 1) = '\0';
  return s;
}

static char *trim(char *s) { return rtrim(ltrim(s)); }

static char *removeComments(char *s) { return strsep(&s, ";"); }

static char *toLower(char *p) {
  char *r = p;
  for (; *p; ++p)
    *p = tolower(*p);
  return r;
}

static uint16_t parseArg(const char *arg) {
  if (strstr(arg, "X") != NULL || strstr(arg, "x") != NULL) {
    int val = (int)strtol(arg, NULL, 16);
    assert(val < UINT16_MAX && val >= 0);
    return val;
  }
  int val = atoi(arg);
  assert(val < UINT16_MAX && val >= 0);
  return val;
}

static int genByteCode(char *inst, uint16_t bCode[2]) {

  char *mnemonic = toLower(strsep(&inst, " "));
  const char *arg = inst;
  printf("'%s': '%s'\n", mnemonic, arg);
  if (strcmp(mnemonic, "push") == 0) {
    assert(arg);
    // 0X444B  -> PUSH 000B
    uint16_t val = parseArg(arg);
    bCode[0] = 0x4400 + val;
    return 1;
  } else if (strcmp(mnemonic, "add") == 0) {
    // 0X4851 -> BINARY 8
    assert(arg == NULL);
    bCode[0] = 0x4851;
    return 1;
  }
  return 0;
}

int EMC_Assemble(const char *srcFilepath, const char *outFilePath) {
  FILE *fileSourceP = fopen(srcFilepath, "r");
  if (fileSourceP == NULL) {
    perror("fopen");
    return 1;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int error = 0;
  int lineNum = 0;

  size_t outBufferSize = 128;
  size_t outBufferIndex = 0;
  uint16_t *outBuffer = malloc(outBufferSize);
  memset(outBuffer, 0, outBufferSize);

  while ((read = getline(&line, &len, fileSourceP)) != -1) {
    if (read > 2) {
      char *trimmedLine = trim(removeComments(line));
      uint16_t bCode[2] = {0};
      int count = genByteCode(trimmedLine, bCode);
      if (!count) {
        error = 1;
        printf("Error at line %i: '%s'\n", lineNum, line);
        break;
      }
      outBuffer[outBufferIndex++] = bCode[0];
      if (outBufferIndex > outBufferSize) {
        outBufferSize *= 2;
        outBuffer = realloc(outBuffer, outBufferSize);
      }
      if (count > 1) {
        outBuffer[outBufferIndex++] = bCode[1];
      }
    }
    lineNum++;
  }

  fclose(fileSourceP);
  if (line)
    free(line);

  for (int i = 0; i < outBufferIndex; i++) {
    if (i % 8 == 0) {
      printf("\n");
    }
    printf("0X%04X ", outBuffer[i]);
  }
  printf("\n");
  FILE *outFileP = fopen(outFilePath, "wb");
  if (outFileP) {
    fwrite(outBuffer, outBufferIndex * 2, 1, outFileP);
    fclose(outFileP);
  }

  printf("oubuffer size=%zi\n", outBufferIndex);
  free(outBuffer);
  return 0;
}
