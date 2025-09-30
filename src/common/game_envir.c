#include "game_envir.h"
#include "formats/format_lang.h"
#include "pak_file.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *dataDir;
  Language lang;
  PAKFile *pakChapter;
} GameEnvironment;

static GameEnvironment _envir;

int GameEnvironmentInit(const char *dataDir) {
  assert(dataDir);
  memset(&_envir, 0, sizeof(GameEnvironment));
  _envir.dataDir = strdup(dataDir);
  _envir.lang = Language_FR;
  printf("GameEnvironmentInit dataDir='%s' lang=%s\n", dataDir,
         LanguageGetExtension(_envir.lang));
  return 1;
}

#define CHAPTER_PREFIX (const char *)"CHAPTER"

int GameEnvironmentLoadChapter(uint8_t index) {
  assert(index < 9);
  const size_t fullPathSize =
      strlen(_envir.dataDir) +
      14; // / sizeof('CHAPTER') + INDEX sizeof('.PAK') + NULL
  char *fullPath = malloc(fullPathSize);
  assert(fullPath);
  assert(snprintf(fullPath, fullPathSize, "%s/%s%i.PAK", _envir.dataDir,
                  CHAPTER_PREFIX, index) < fullPathSize);
  printf("GameEnvironmentLoadChapter load %i '%s'\n", index, fullPath);

  assert(_envir.pakChapter == NULL); // TODO release previous chapter if loaded
  _envir.pakChapter = malloc(sizeof(PAKFile));
  assert(_envir.pakChapter);
  PAKFileInit(_envir.pakChapter);
  int ret = PAKFileRead(_envir.pakChapter, fullPath);
  free(fullPath);
  return ret;
}

void GameEnvironmentRelease(void) {
  free((void *)_envir.dataDir);
  if (_envir.pakChapter) {
    PAKFileRelease(_envir.pakChapter);
    free(_envir.pakChapter);
  }
}

int GameEnvironmentGetFile(GameFile *file, const char *name) {
  assert(file);
  assert(name);
  printf("GameEnvironmentGetFile: '%s'\n", name);
  if (_envir.pakChapter == NULL) {
    return 0;
  }
  int index = PakFileGetEntryIndex(_envir.pakChapter, name);
  if (index == -1) {
    printf("GameEnvironmentGetFile no such file '%s'\n", name);
    return 0;
  }
  size_t size = _envir.pakChapter->entries[index].fileSize;
  if (size) {
    file->buffer = PakFileGetEntryData(_envir.pakChapter, index);
    file->bufferSize = _envir.pakChapter->entries[index].fileSize;
    return 1;
  }
  return 0;
}

int GameEnvironmentGetLangFile(GameFile *file, const char *name) {
  assert(file);
  assert(name);
  const size_t fullNameSize = strlen(name) + 5;
  char *fullName = malloc(fullNameSize);
  assert(fullName);
  assert(snprintf(fullName, fullNameSize, "%s.%s", name,
                  LanguageGetExtension(_envir.lang)) < fullNameSize);
  int ret = GameEnvironmentGetFile(file, fullName);
  free(fullName);
  return ret;
}
