
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 13
typedef struct {
  uint32_t offset;
  char filename[MAX_FILENAME];

  // computed when parsing
  uint32_t fileSize;
} PAKFileEntry;

typedef struct {
  PAKFileEntry *entries;
  int count;
} PAKFile;

static int readPAKFileEntry(FILE *fp, PAKFileEntry *entry) {
  fread(&entry->offset, sizeof(uint32_t), 1, fp);

  memset(entry->filename, 0, MAX_FILENAME);
  for (int i = 0; i < MAX_FILENAME; i++) {
    fread(&entry->filename[i], 1, 1, fp);

    if (entry->filename[i] == 0) {
      break;
    }
  }
  assert(entry->filename[MAX_FILENAME - 1] == 0);
  return 0;
}

static int readPAKFile(PAKFile *file, const char *filepath) {
  FILE *fp = fopen(filepath, "r");
  if (fp == NULL) {
    return 0;
  }

  file->count = 0;
  while (1) {
    file->entries =
        realloc(file->entries, (file->count + 1) * sizeof(PAKFileEntry));
    readPAKFileEntry(fp, &file->entries[file->count]);
    if (file->count > 0) {
      file->entries[file->count - 1].fileSize =
          file->entries[file->count].offset -
          file->entries[file->count - 1].offset;
    }

    if (file->entries[file->count].filename[0] == 0) { // last entry
      break;
    }
    file->count++;
  }
  file->count--;
  fclose(fp);
  return 1;
}

int main(int argc, char *argv[]) {
  PAKFile file;
  file.entries = NULL;
  if (readPAKFile(
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
  free(file.entries);
  return 0;
}
