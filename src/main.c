
#include "bytes.h"
#include "emc_asm.h"
#include "format_cmz.h"
#include "format_cps.h"
#include "format_inf.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include "pak_file.h"
#include "renderer.h"
#include "tests.h"
#include <_string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmdWLL(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);

  TestWLL(buffer, fileSize);
  free(buffer);
  return 0;
}

static int cmdVCN(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);
  VCNHandle handle = {0};
  if (!VCNHandleFromLCWBuffer(&handle, buffer, fileSize)) {
    printf("VCNDataFromLCWBuffer error\n");
    return 1;
  }
  const char *outFile = "out.png";
  printf("Write VCN image '%s'\n", outFile);
  VCNImageToPng(&handle, outFile);
  VCNHandleRelease(&handle);
  return 0;
}

static int cmdVMP(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);
  VMPHandle handle = {0};
  if (!VMPHandleFromLCWBuffer(&handle, buffer, fileSize)) {
    printf("VMPDataFromLCWBuffer error\n");
    return 1;
  }
  VMPHandleTest(&handle);
  VMPHandleRelease(&handle);
  return 0;
}

static int cmdScriptExec(const char *filepath) {
  printf("asm script '%s'\n", filepath);
  return EMC_Exec(filepath);
}

static int cmdScriptDisassemble(const char *filepath) {
  char *outFilePath = malloc(strlen(filepath) + 5);
  if (outFilePath == NULL) {
    return 1;
  }
  strcpy(outFilePath, filepath);
  outFilePath = strcat(outFilePath, ".asm");
  printf("disasm script '%s' to file '%s'\n", filepath, outFilePath);
  int ret = EMC_DisassembleFile(filepath, outFilePath);
  free(outFilePath);
  return ret;
}

static int cmdScriptASM(const char *filepath) {
  char *outFilePath = malloc(strlen(filepath) + 5);
  if (outFilePath == NULL) {
    return 1;
  }
  strcpy(outFilePath, filepath);
  outFilePath = strcat(outFilePath, ".bin");
  printf("asm script '%s' to file '%s'\n", filepath, outFilePath);
  int ret = EMC_AssembleFile(filepath, outFilePath);
  free(outFilePath);
  return ret;
}

static int cmdScriptTests(void) { return EMC_Tests(); }

static void usageScript(void) {
  printf("script subcommands: asm|exec|tests|disasm [filepath]\n");
}

static int cmdScript(int argc, char *argv[]) {
  if (argc < 1) {
    printf("script command, missing arguments\n");
    usageScript();
    return 1;
  }

  const char *filepath = argv[1];

  if (strcmp(argv[0], "exec") == 0) {
    if (argc < 2) {
      printf("script: missing file path\n");
      return 1;
    }
    return cmdScriptExec(filepath);
  } else if (strcmp(argv[0], "asm") == 0) {
    if (argc < 2) {
      printf("script: missing file path\n");
      return 1;
    }
    return cmdScriptASM(filepath);
  } else if (strcmp(argv[0], "disasm") == 0) {
    return cmdScriptDisassemble(filepath);
  } else if (strcmp(argv[0], "tests") == 0) {
    return cmdScriptTests();
  }
  printf("unkown subcommand '%s'\n", argv[0]);
  usageScript();
  return 1;
}

static int cmdMap(int argc, char *argv[]) {
  // TODO: use readBinaryFile
  FILE *f = fopen(argv[0], "rb");
  if (!f) {
    perror("open: ");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("malloc error");
    fclose(f);
    return 1;
  }
  fread(buffer, fsize, 1, f);
  fclose(f);

  TestCMZ(buffer, fsize);
  return 0;
}

static void usageCPS(void) { printf("cps cpsfile \n"); }

static int cmdCPS(int argc, char *argv[]) {
  if (argc < 1) {
    usageCPS();
    return 1;
  }
  const char *cpsFile = argv[0];
  FILE *inFile = fopen(cpsFile, "rb");
  if (!inFile) {
    perror("open: ");
    return 1;
  }
  fseek(inFile, 0, SEEK_END);
  long inFileSize = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);
  uint8_t *buffer = malloc(inFileSize);
  if (!buffer) {
    perror("malloc error");
    fclose(inFile);
    return 1;
  }
  fread(buffer, inFileSize, 1, inFile);
  fclose(inFile);
  CPSImage image;
  int ok = CPSImageFromFile(&image, buffer, inFileSize);
  free(buffer);
  if (ok) {
    char *destFile = strdup(cpsFile);
    assert(destFile);
    destFile[strlen(destFile) - 3] = 'p';
    destFile[strlen(destFile) - 2] = 'n';
    destFile[strlen(destFile) - 1] = 'g';
    CPSImageToPng(&image, destFile);
    free(destFile);
    CPSImageRelease(&image);
  }

  return !ok;
}

static void usageCMZ(void) { printf("cmz subcommands: unzip cpsFilepath\n"); }

static int cmdCMZUnzip(const char *cmzfilePath) {
  printf("unzip cmz '%s'\n", cmzfilePath);
  FILE *inFile = fopen(cmzfilePath, "rb");
  if (!inFile) {
    perror("open: ");
    return 1;
  }
  fseek(inFile, 0, SEEK_END);
  long inFileSize = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);
  uint8_t *buffer = malloc(inFileSize);
  if (!buffer) {
    perror("malloc error");
    fclose(inFile);
    return 1;
  }
  fread(buffer, inFileSize, 1, inFile);
  fclose(inFile);
  size_t unzipedDataSize = 0;
  printf("CMZ_Uncompress %zi bytes\n", inFileSize);
  uint8_t *unzipedData = CMZDecompress(buffer, inFileSize, &unzipedDataSize);
  int err = 0;
  if (unzipedData == NULL) {
    printf("error while unzipping file\n");
    err = 1;
  } else {
    char *outFilePath = malloc(strlen(cmzfilePath) + 5);
    strcpy(outFilePath, cmzfilePath);
    outFilePath = strcat(outFilePath, ".maz");
    FILE *outFile = fopen(outFilePath, "wb");
    if (outFile) {
      fwrite(unzipedData, unzipedDataSize, 1, outFile);
      fclose(outFile);
    }
    free(outFilePath);
  }

  free(buffer);
  free(unzipedData);
  return err;
}

static void usageINF(void) {
  printf("inf subcommands: extract|show infFile\n");
}

static int cmdINFShowContent(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (!f) {
    perror("open: ");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("malloc error");
    fclose(f);
    return 1;
  }
  fread(buffer, fsize, 1, f);
  fclose(f);
  INFScript script;
  INFScriptInit(&script);
  INFScriptFromBuffer(&script, buffer, fsize);
  printf("INF Content from file '%s':\n", filepath);
  printf("\t%zi instructions\n", INFScriptGetCodeBinarySize(&script));
  INFScriptListText(&script);
  printf("Script function segments:\n");
  INFScriptListScriptFunctions(&script);
  INFScriptRelease(&script);
  return 0;
}

static int cmdINFExtractCode(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (!f) {
    perror("open: ");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("malloc error");
    fclose(f);
    return 1;
  }
  fread(buffer, fsize, 1, f);
  fclose(f);
  INFScript script;
  INFScriptInit(&script);
  INFScriptFromBuffer(&script, buffer, fsize);

  size_t numInstructions = INFScriptGetCodeBinarySize(&script);
  const uint16_t *instructions = INFScriptGetCodeBinary(&script);

  char outFilePath[64] = "";
  assert(snprintf(outFilePath, 64, "%s.script.bin", filepath) < 64);

  FILE *outFile = fopen(outFilePath, "wb");
  if (outFile) {
    fwrite(instructions, numInstructions * 2, 1, outFile);
    fclose(outFile);
  }

  INFScriptRelease(&script);
  return 0;
}

static int cmdINF(int argc, char *argv[]) {
  if (argc < 1) {
    printf("inf command, missing arguments\n");
    usageINF();
    return 1;
  }
  if (strcmp(argv[0], "extract") == 0 && argc == 2) {
    return cmdINFExtractCode(argv[1]);
  } else if (strcmp(argv[0], "show") == 0 && argc == 2) {
    return cmdINFShowContent(argv[1]);
  }
  usageINF();
  return 1;
}

static int cmdCMZ(int argc, char *argv[]) {
  if (argc < 1) {
    printf("cmz command, missing arguments\n");
    usageCMZ();
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "unzip") == 0) {
    if (argc < 2) {
      printf("cmz unzip: missing cmz file path\n");
      return 1;
    }

    return cmdCMZUnzip(filepath);
  }
  printf("unkown subcommand '%s'\n", argv[0]);
  usageCMZ();
  return 1;
}

static void usagePak(void) {
  printf("pak subcommands: list|extract pakFilepath [file]\n");
}

static int cmdPakList(const char *pakfilepath) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(&file, pakfilepath) == 0) {
    perror("Error while reading file");
    return 1;
  }

  for (int i = 0; i <= file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", i, i,
           entry->offset, entry->filename, PakFileEntryGetExtension(entry),
           entry->fileSize);
  }

  PAKFileRelease(&file);
  return 0;
}

static int cmdPakExtract(const char *pakfilepath, const char *fileToShow) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(&file, pakfilepath) == 0) {
    perror("Error while reading file");
    return 1;
  }

  int found = 0;
  int ok = 0;
  for (int i = 0; i < file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    if (strcmp(entry->filename, fileToShow) == 0) {
      printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", i, i,
             entry->offset, entry->filename, PakFileEntryGetExtension(entry),
             entry->fileSize);
      found = 1;

      ok = PakFileExtract(&file, entry, fileToShow);
      break;
    }
  }

  PAKFileRelease(&file);
  if (!found) {
    printf("File '%s' not found in PAK\n", fileToShow);
    return 1;
  }
  return ok;
}

static int cmdPak(int argc, char *argv[]) {
  if (argc < 1) {
    printf("pak command, missing arguments\n");
    usagePak();
    return 1;
  }
  if (argc < 2) {
    printf("pak list: missing pak file path\n");
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "list") == 0) {
    return cmdPakList(filepath);
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 3) {
      printf("pak extract: missing file name \n");
      return 1;
    }
    return cmdPakExtract(filepath, argv[2]);
  }

  printf("Unknown pak command '%s'\n", argv[0]);
  usagePak();
  return 1;
}

static void usage(const char *progName) {
  printf("%s: pak|cmz|map|script|inf|tests|wll subcommand ...\n", progName);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }
  if (strcmp(argv[1], "pak") == 0) {
    return cmdPak(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cmz") == 0) {
    return cmdCMZ(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "wll") == 0) {
    return cmdWLL(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "vcn") == 0) {
    return cmdVCN(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "vmp") == 0) {
    return cmdVMP(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "inf") == 0) {
    return cmdINF(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cps") == 0) {
    return cmdCPS(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "map") == 0) {
    return cmdMap(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "script") == 0) {
    return cmdScript(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "tests") == 0) {
    return UnitTests();
  }
  printf("Unknown command '%s'\n", argv[1]);
  usage(argv[0]);
  return 1;
}
