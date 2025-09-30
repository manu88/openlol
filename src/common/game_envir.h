#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  uint8_t *buffer;
  size_t bufferSize;
} GameFile;

int GameEnvironmentInit(const char *dataDir);
void GameEnvironmentRelease(void);

int GameEnvironmentLoadChapter(uint8_t index);

int GameEnvironmentGetFile(GameFile *file, const char *name);

int GameEnvironmentGetFileWithExt(GameFile *file, const char *name, const char* ext);
int GameEnvironmentGetLangFile(GameFile *file, const char *name);

