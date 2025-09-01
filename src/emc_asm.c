#include "emc_asm.h"
#include <_string.h>
#include <stdint.h>
#define _GNU_SOURCE
#include "bytes.h"
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
  const char *mnemonic = toLower(strsep(&inst, " "));
  const char *arg = inst;
  printf("'%s': '%s'\n", mnemonic, arg);
  if (strcmp(mnemonic, "push") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    if (val < 0X80) {
      // emit OP_PUSH
      // 0X4144 -> push 1
      bCode[0] = 0x0044 + (val << 8);
      return 1;
    } else {
      // emit OP_PUSH2
      bCode[0] = 0X0423;
      bCode[1] = swap_uint16(val);
      return 2;
    }

  } else if (strcmp(mnemonic, "add") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0851;
    return 1;
  } else if (strcmp(mnemonic, "multiply") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0A51;
    return 1;
  }
  return 0;
}

uint16_t *EMC_Assemble(const char *script, size_t *retOutBufferSize) {
  size_t outBufferSize = 128;
  size_t outBufferIndex = 0;
  uint16_t *outBuffer = malloc(outBufferSize);
  memset(outBuffer, 0, outBufferSize);
  int error = 0;
  int lineNum = 0;
  char *line = strtok((char *)script, "\n");
  while (line != NULL) {
    printf("line '%s'\n", line);
    size_t lineSize = strlen(line);

    if (lineSize > 2) {
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
    line = strtok(NULL, "\r\n");
    lineNum++;
  }

  if (error != 0) {
    return NULL;
  }
  *retOutBufferSize = outBufferIndex;
  return outBuffer;
}

int EMC_AssembleFile(const char *srcFilepath, const char *outFilePath) {
  FILE *fileSourceP = fopen(srcFilepath, "r");
  if (fileSourceP == NULL) {
    perror("fopen");
    return 1;
  }

  size_t outBufferSize = 128;
  size_t outBufferIndex = 0;
  uint16_t *outBuffer = malloc(outBufferSize);
  memset(outBuffer, 0, outBufferSize);

  char *line = NULL;
  size_t len = 0;
  ssize_t lineSize;
  int error = 0;
  int lineNum = 0;

  while ((lineSize = getline(&line, &len, fileSourceP)) != -1) {
    if (lineSize > 2) {
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

  if (error != 0) {
    free(outBuffer);
    return error;
  }

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

int EMC_Exec(const char *scriptFilepath) {
  FILE *fp = fopen(scriptFilepath, "r");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }
  fseek(fp, 0, SEEK_END);
  size_t fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("malloc error");
    fclose(fp);
    return 1;
  }
  fread(buffer, fsize, 1, fp);
  fclose(fp);
  printf("script binary size = %zu\n", fsize);

  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = fsize / 2;
  ScriptExec(&vm, &inf);
  ScriptVMDump(&vm);
  free(buffer);
  return 0;
}

static uint16_t basicBinaryTest(const char *s) {
  size_t bufferSize = 0;
  uint16_t *buffer = EMC_Assemble(s, &bufferSize);
  assert(bufferSize == 3);
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = bufferSize;
  ScriptExec(&vm, &inf);
  free(buffer);
  return vm.stack[vm.stackPointer];
}

static uint16_t basicPush(uint16_t pushedVal) {
  size_t bufferSize = 0;
  char s[32] = "0";
  snprintf(s, 32, "push 0X%X", pushedVal);
  uint16_t *buffer = EMC_Assemble(s, &bufferSize);
  assert(bufferSize < 3);
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = bufferSize;
  ScriptExec(&vm, &inf);
  free(buffer);
  return vm.stack[vm.stackPointer];
}

static void expect(uint16_t val, uint16_t expected) {
  if (val != expected) {
    printf("Expected 0X%0X got OX%0X\n", expected, val);
    assert(0);
  }
}

int EMC_Tests(void) {
  for (uint16_t i = 0; i < UINT16_MAX; i++) {
    uint16_t v = basicPush(i);
    if (v != i) {
      printf("basicPush(0X%X), expected 0X%X got 0X%X\n", i, i, v);
      assert(0);
    }
  }
  {
    const char s[] = "Push 0X1B ; some comments\nPush 0X02\nAdd\n";
    expect(basicBinaryTest(s), 0X1D);
  }
  {
    const char s[] = "Push 0X02 ; some comments\nPush 0X04\nMultiply\n";
    expect(basicBinaryTest(s), 0X08);
  }
  return 0;
}
