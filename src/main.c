
#include "format_cps.h"
#include "pak_file.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usagePak(void) { printf("pak subcommands: list\n"); }

static int cmdPakList(const char *pakfilepath) {
  printf("List for '%s'\n", pakfilepath);
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(&file, pakfilepath) == 0) {
    perror("Error while reading file");
    return 1;
  }
  printf("read %i entries\n", file.count);

  for (int i = 0; i <= file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    printf("%i: Entry offset %u name '%s' ('%s') size %u \n", i, entry->offset,
           entry->filename, PakFileEntryGetExtension(entry), entry->fileSize);
  }

  PAKFileRelease(&file);
  return 0;
}

static int cmdPak(int argc, char *argv[]) {
  if (argc < 1) {
    printf("pak command, missing arguments\n");
    usagePak();
    return 1;
  }
  if (strcmp(argv[0], "list") == 0) {
    if (argc < 2) {
      printf("pak list: missing pak file path\n");
      return 1;
    }
    return cmdPakList(argv[1]);
  }
  printf("Unknown pak command '%s'\n", argv[0]);
  usagePak();
  return 1;
}

static void usage(const char *progName) {
  printf("%s: pak subcommand ...\n", progName);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }
  if (strcmp(argv[1], "pak") == 0) {
    return cmdPak(argc - 2, argv + 2);
  }
  printf("Unknown command '%s'\n", argv[1]);
  usage(argv[0]);
  return 1;
}
