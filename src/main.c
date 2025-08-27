
#include "pak_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(
          &file,
          "/Users/manueldeneu/Downloads/setup-00399-Lands_of_Lore-PCLinux20/"
          "sources/C/LandsOL/CHAPTER1.PAK") == 0) {
    perror("Error while reading file");
    return 1;
  }
  printf("read %i entries\n", file.count);

  for (int i = 0; i <= file.count; i++) {
    printf("%i: Entry offset %u name '%s' size %u \n", i,
           file.entries[i].offset, file.entries[i].filename,
           file.entries[i].fileSize);
  }
  PAKFileRelease(&file);
  return 0;
}
