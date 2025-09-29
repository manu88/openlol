#pragma once
#include <stdint.h>
#include <stdio.h>

#define MAX_FILENAME 13
typedef struct {
  uint32_t offset;
  char filename[MAX_FILENAME];

  // computed when parsing
  uint32_t fileSize;

  // content cache
  uint8_t *data;
} PAKEntry;

typedef struct {
  PAKEntry *entries;
  int count;

  FILE *fp;
} PAKFile;

void PAKFileInit(PAKFile *file);
void PAKFileRelease(PAKFile *file);
int PAKFileRead(PAKFile *file, const char *filepath);

int PakFileGetEntryIndex(const PAKFile *file, const char *name);

const char *PakFileEntryGetExtension(const PAKEntry *entry);

uint8_t *PakFileGetEntryData(const PAKFile *file, int index);

const PAKFile *PakFileGetMain(void);
int PakFileLoadMain(const char *filepath);
void PakFileReleaseMain(void);
