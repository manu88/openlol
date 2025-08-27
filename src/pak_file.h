#pragma once
#include <stdint.h>

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

void PAKFileInit(PAKFile *file);
void PAKFileRelease(PAKFile *file);
int PAKFileRead(PAKFile *file, const char *filepath);
