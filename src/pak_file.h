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

int PakFileExtract(PAKFile *file, PAKEntry *entry, const char *toFile);
const char *PakFileEntryGetExtension(const PAKEntry *entry);

uint8_t *PakFileGetEntryData(const PAKFile *file, PAKEntry *entry);
