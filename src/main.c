
#include "emc_asm.h"
#include "format_inf.h"
#include "pak_file.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmdScriptExec(const char *filepath) {
  printf("asm script '%s'\n", filepath);
  return 0;
}

static int cmdScriptASM(const char *filepath) {
  char *outFilePath = malloc(strlen(filepath) + 5);
  strcpy(outFilePath, filepath);
  outFilePath = strcat(outFilePath, ".bin");
  printf("asm script '%s' to file '%s'\n", filepath, outFilePath);
  int ret = EMC_Assemble(filepath, outFilePath);
  free(outFilePath);
  return ret;
}

static void usageScript(void) {
  printf("script subcommands: asm|exec filepath\n");
}

static int cmdScript(int argc, char *argv[]) {
  if (argc < 1) {
    printf("script command, missing arguments\n");
    usageScript();
    return 1;
  }
  if (argc < 2) {
    printf("script: missing file path\n");
    return 1;
  }
  const char *filepath = argv[1];

  if (strcmp(argv[0], "exec") == 0) {
    return cmdScriptExec(filepath);
  } else if (strcmp(argv[0], "asm") == 0) {
    return cmdScriptASM(filepath);
  }
  printf("unkown subcommand '%s'\n", argv[0]);
  usageScript();
  return 1;
}

static int cmdMap(int argc, char *argv[]) {
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
  printf("read %zi bytes\n", fsize);
  TestINF(buffer, fsize);
  fclose(f);
  free(buffer);
  return 0;
}

static void usageCPS(void) { printf("cps subcommands: extract cpsFilepath\n"); }

static int cmdCPSExtract(const char *cpsfilePath) {
  printf("Extract CPS '%s'\n", cpsfilePath);
  return 0;
}

static int cmdCPS(int argc, char *argv[]) {
  if (argc < 1) {
    printf("cps command, missing arguments\n");
    usageCPS();
    return 1;
  }
  if (argc < 2) {
    printf("cps list: missing cps file path\n");
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "extract") == 0) {
    return cmdCPSExtract(filepath);
  }
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
    printf("%i: Entry offset %u name '%s' ('%s') size %u \n", i, entry->offset,
           entry->filename, PakFileEntryGetExtension(entry), entry->fileSize);
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
  for (int i = 0; i <= file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    if (strcmp(entry->filename, fileToShow) == 0) {
      printf("%i: Entry offset %u name '%s' ('%s') size %u \n", i,
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
  printf("%s: pak|cps|map subcommand ...\n", progName);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }
  if (strcmp(argv[1], "pak") == 0) {
    return cmdPak(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cps") == 0) {
    return cmdCPS(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "map") == 0) {
    return cmdMap(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "script") == 0) {
    return cmdScript(argc - 2, argv + 2);
  }
  printf("Unknown command '%s'\n", argv[1]);
  usage(argv[0]);
  return 1;
}
