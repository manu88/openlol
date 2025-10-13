#include "pak_file.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void PAKFileInit(PAKFile *file) { memset(file, 0, sizeof(PAKFile)); }
void PAKFileRelease(PAKFile *file) {
  fclose(file->fp);
  for (int i = 0; i < file->count; i++) {
    if (file->entries[i].data) {
      free(file->entries[i].data);
    }
  }
  free(file->entries);
}

static int readPAKFileEntry(FILE *fp, PAKEntry *entry) {
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
  file->fp = fopen(filepath, "r");
  if (file->fp == NULL) {
    return 0;
  }

  file->count = 0;
  while (1) {
    file->entries =
        realloc(file->entries, (file->count + 1) * sizeof(PAKEntry));
    memset(file->entries + file->count, 0, sizeof(PAKEntry));
    assert(file->entries[file->count].data == NULL);
    readPAKFileEntry(file->fp, &file->entries[file->count]);
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
  return 1;
}

const char *PakFileEntryGetExtension(const PAKEntry *entry) {
  return entry->filename + strlen(entry->filename) - 3;
}

int PakFileGetEntryIndex(const PAKFile *file, const char *name) {
  for (int i = 0; i <= file->count; i++) {
    if (strcmp(name, file->entries[i].filename) == 0) {
      return i;
    }
  }
  return -1;
}

uint8_t *PakFileGetEntryData(const PAKFile *file, int index) {
  if (index < 0 || index >= file->count) {
    printf("Will assert for %i %i\n", index, file->count);
  }
  assert(index >= 0 && index < file->count);
  PAKEntry *entry = file->entries + index;
  if (entry->data == NULL) {
    fseek(file->fp, entry->offset, SEEK_SET);
    entry->data = malloc(entry->fileSize);
    if (fread(entry->data, entry->fileSize, 1, file->fp) != 1) {
      perror("PakFileGetEntryData.fread");
      return NULL;
    }
  }
  return entry->data;
}

static PAKFile _mainPak;
static int _mainPakLoaded = 0;
const PAKFile *PakFileGetMain(void) {
  if (!_mainPakLoaded) {
    return NULL;
  }
  return &_mainPak;
}

int PakFileLoadMain(const char *filepath) {
  _mainPakLoaded = 0;
  PAKFileInit(&_mainPak);
  if (!PAKFileRead(&_mainPak, filepath)) {
    return 0;
  }
  _mainPakLoaded = 1;
  return 1;
}

void PakFileReleaseMain(void) {
  if (_mainPakLoaded) {
    PAKFileRelease(&_mainPak);
  }
}
