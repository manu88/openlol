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
int GameEnvironmentGetLangFile(GameFile *file, const char *name);

