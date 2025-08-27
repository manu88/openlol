#include "pak_file.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void PAKFileInit(PAKFile *file) { memset(file, 0, sizeof(PAKFile)); }
void PAKFileRelease(PAKFile *file) { free(file->entries); }

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

int PAKFileRead(PAKFile *file, const char *filepath) {
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
