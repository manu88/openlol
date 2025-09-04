#include "emc_asm.h"
#include <_string.h>
#include <stddef.h>
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
    if (val >= UINT16_MAX + 1 || val < 0) {
      printf("invalid val %i %x\n", val, val);
    }
    assert(val < UINT16_MAX + 1 && val >= 0);
    return val;
  }
  int val = atoi(arg);
  assert(val < UINT16_MAX && val >= 0);
  return val;
}

static int genByteCode(char *inst, uint16_t bCode[2]) {
  const char *mnemonic = toLower(strsep(&inst, " "));
  const char *arg = inst;
  if (strcmp(mnemonic, "push") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    if (val < 0X80) {
      // emit OP_PUSH
      bCode[0] = 0x0044 + (val << 8);
      return 1;
    }
    // emit OP_PUSH2
    bCode[0] = 0X0423;
    bCode[1] = swap_uint16(val);
    return 2;
  } else if (strcmp(mnemonic, "l_or") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_LogicalOR << 8);
    return 1;
  } else if (strcmp(mnemonic, "add") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Add << 8);
    return 1;
  } else if (strcmp(mnemonic, "xor") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_XOR << 8);
    return 1;
  } else if (strcmp(mnemonic, "or") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_OR << 8);
    return 1;
  } else if (strcmp(mnemonic, "lshift") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_LShift << 8);
    return 1;
  } else if (strcmp(mnemonic, "and") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_AND << 8);
    return 1;
  } else if (strcmp(mnemonic, "inf") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Inf << 8);
    return 1;
  } else if (strcmp(mnemonic, "l_and") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_LogicalAND << 8);
    return 1;
  } else if (strcmp(mnemonic, "and") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_AND << 8);
    return 1;
  } else if (strcmp(mnemonic, "sup") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Greater << 8);
    return 1;
  } else if (strcmp(mnemonic, "minus") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Minus << 8);
    return 1;
  } else if (strcmp(mnemonic, "eq") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_EQUAL << 8);
    return 1;
  } else if (strcmp(mnemonic, "neq") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_NotEQUAL << 8);
    return 1;
  } else if (strcmp(mnemonic, "multiply") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Multiply << 8);
    return 1;
  } else if (strcmp(mnemonic, "rshift") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_RShift << 8);
    return 1;
  } else if (strcmp(mnemonic, "divide") == 0) {
    assert(arg == NULL);
    bCode[0] = 0x0051 + (BinaryOp_Divide << 8);
    return 1;
  } else if (strcmp(mnemonic, "pusharg") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x0047 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "call") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x004E + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "stackrewind") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x004C + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "pop") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x0049 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "pushvar") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x0045 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "poprc") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x0048 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "pushrc") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    if (val != 0 && val != 1) {
      printf("invalid value: 0X%X\n", val);
      return 0;
    }
    bCode[0] = 0x0042 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "ifnotgo") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0X002F;
    bCode[1] = swap_uint16(val);
    return 2;
  } else if (strcmp(mnemonic, "stackforward") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x004D + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "poplocvar") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x004A + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "pushlocvar") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0x0046 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "jump") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    bCode[0] = 0X0080 + (val << 8);
    return 1;
  } else if (strcmp(mnemonic, "unary") == 0) {
    assert(arg);
    uint16_t val = parseArg(arg);
    if (val != 0 && val != 1) {
      printf("invalid value: 0X%X\n", val);
      return 0;
    }
    bCode[0] = 0X0050 + (val << 8);
    return 1;
  }
  return 0;
}

uint16_t *EMC_Assemble(const char *script, size_t *retOutBufferSize) {
  size_t outBufferSize = 128;
  size_t outBufferIndex = 0;
  uint16_t *outBuffer = malloc(outBufferSize * 2);
  memset(outBuffer, 0, outBufferSize);
  int error = 0;
  int lineNum = 0;
  char *originalLine = strtok((char *)script, "\n");
  while (originalLine != NULL) {
    char *line = strdup(originalLine);
    size_t lineSize = strlen(line);

    if (lineSize > 2) {
      char *trimmedLine = trim(removeComments(line));
      uint16_t bCode[2] = {0};
      int count = genByteCode(trimmedLine, bCode);
      if (!count) {
        error = 1;
        printf("Error at line %i: '%s'\n", lineNum, originalLine);
        free(originalLine);
        break;
      }
      if (outBufferIndex >= outBufferSize) {

        outBufferSize *= 2;
        outBuffer = realloc(outBuffer, outBufferSize * 2);
      }
      outBuffer[outBufferIndex++] = bCode[0];
      if (count > 1) {
        outBuffer[outBufferIndex++] = bCode[1];
      }
    }
    originalLine = strtok(NULL, "\r\n");
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
  fseek(fileSourceP, 0, SEEK_END);
  long inFileSize = ftell(fileSourceP);
  fseek(fileSourceP, 0, SEEK_SET);

  char *script = malloc(inFileSize);
  if (!script) {
    fclose(fileSourceP);
    return 1;
  }
  if (fread(script, inFileSize, 1, fileSourceP) != 1) {
    perror("fread");
    fclose(fileSourceP);
    free(script);
    return 1;
  }
  fclose(fileSourceP);

  size_t outBufferSize;
  uint16_t *outBuffer = EMC_Assemble(script, &outBufferSize);

  FILE *outFileP = fopen(outFilePath, "wb");
  if (outFileP) {
    fwrite(outBuffer, outBufferSize * 2, 1, outFileP);
    fclose(outFileP);
  }
  free(outBuffer);
  return 0;
}

int EMC_DisassembleFile(const char *binFilepath, const char *outFilePath) {
  FILE *fileBinP = fopen(binFilepath, "rb");
  if (fileBinP == NULL) {
    perror("fopen");
    return 1;
  }
  fseek(fileBinP, 0, SEEK_END);
  long fsize = ftell(fileBinP);
  fseek(fileBinP, 0, SEEK_SET);
  uint8_t *binBuffer = malloc(fsize);
  if (!binBuffer) {
    perror("malloc error");
    fclose(fileBinP);
    return 1;
  }
  fread(binBuffer, fsize, 1, fileBinP);
  fclose(fileBinP);
  printf("read %zi bytes\n", fsize);

  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptVMSetDisassembler(&vm);
  vm.showDisamComment = 1;
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)binBuffer;
  inf.scriptSize = fsize / 2;
  assert(vm.disasmBuffer);
  if (!ScriptExec(&vm, &inf)) {
    printf("error while disassembling code\n");
    return 1;
  }

  printf("Wrote %zi bytes of assembly code\n", vm.disasmBufferIndex);

  FILE *outFileP = fopen(outFilePath, "w");
  if (outFileP) {
    fwrite(vm.disasmBuffer, strlen(vm.disasmBuffer), 1, outFileP);
    fclose(outFileP);
  }

  ScriptVMRelease(&vm);
  free(binBuffer);
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

  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = fsize / 2;
  ScriptExec(&vm, &inf);
  ScriptVMDump(&vm);
  ScriptVMRelease(&vm);
  free(buffer);
  return 0;
}

static uint16_t basicBinaryTest(const char *s) {
  size_t bufferSize = 0;
  uint16_t *buffer = EMC_Assemble(s, &bufferSize);
  assert(buffer);
  assert(bufferSize > 0);
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = bufferSize;
  ScriptExec(&vm, &inf);
  ScriptVMRelease(&vm);
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
  assert(ScriptExec(&vm, &inf));
  ScriptVMRelease(&vm);
  free(buffer);
  return vm.stack[vm.stackPointer];
}

static void expect(uint16_t val, uint16_t expected) {
  if (val != expected) {
    printf("Expected 0X%0X got OX%0X\n", expected, val);
    assert(0);
  }
}

static void TestInstruction(const char *sOrigin, size_t instructionsCount) {
  char *s = strdup(sOrigin);
  size_t bufferSize = 0;
  uint16_t *buffer = EMC_Assemble(s, &bufferSize);
  if (bufferSize != instructionsCount) {
    printf("'%s': Buffersize=%zu, expected %zu\n", sOrigin, bufferSize,
           instructionsCount);
  }
  ScriptVM vm;
  ScriptVMInit(&vm);
  ScriptVMSetDisassembler(&vm);
  ScriptInfo inf;
  inf.scriptData = (uint16_t *)buffer;
  inf.scriptSize = bufferSize;
  assert(ScriptExec(&vm, &inf));
  assert(vm.disasmBuffer);
  assert(vm.disasmBufferSize);

  if (strcmp(sOrigin, vm.disasmBuffer) != 0) {

    printf("input  (%lu) : '%s'\n", strlen(sOrigin), sOrigin);
    printf("output (%lu) : '%s'\n", strlen(vm.disasmBuffer), vm.disasmBuffer);
    assert(0);
  }
  assert(strlen(sOrigin) == strlen(vm.disasmBuffer));

  ScriptVMRelease(&vm);
  free(buffer);
  free(s);
}

int EMC_Tests(void) {
  printf("EMC_Tests\n");

  TestInstruction("PushArg 0X1\n", 1);
  TestInstruction("call 0X59\n", 1);
  TestInstruction("StackRewind 0X1\n", 1);
  TestInstruction("PUSH 0XFFFF\n", 2);

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
  {
    const char s[] = "Push 0X0F\nPush 0X03\ndivide\n";
    expect(basicBinaryTest(s), 0X05);
  }
  {
    const char s[] =
        "Push 0X3C\nPush 0X05\nmultiply\npush 0XA\npush 5\nAdd\ndivide\n";
    expect(basicBinaryTest(s), 0X14);
  }
  return 0;
}
