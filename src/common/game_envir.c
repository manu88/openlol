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
  PAKFile pakGeneral;
} GameEnvironment;

static GameEnvironment _envir;

static const char generalPakName[] = "GENERAL.PAK";

int GameEnvironmentInit(const char *dataDir) {
  assert(dataDir);
  memset(&_envir, 0, sizeof(GameEnvironment));
  _envir.dataDir = strdup(dataDir);
  _envir.lang = Language_FR;
  printf("GameEnvironmentInit dataDir='%s' lang=%s\n", dataDir,
         LanguageGetExtension(_envir.lang));

  PAKFileInit(&_envir.pakGeneral);

  size_t s = strlen(_envir.dataDir) + 2 + strlen(generalPakName);
  char *generalPath = malloc(s);
  assert(generalPath);
  snprintf(generalPath, s, "%s/%s", _envir.dataDir, generalPakName);
  printf("open general Pak at '%s'\n", generalPath);
  assert(PAKFileRead(&_envir.pakGeneral, generalPath));
  free(generalPath);
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
  PAKFileRelease(&_envir.pakGeneral);
}

int GameEnvironmentGetGeneralFile(GameFile *file, const char *name) {
  int index = PakFileGetEntryIndex(&_envir.pakGeneral, name);
  if (index == -1) {
    printf("GameEnvironmentGetFile no such file '%s' in %s\n", name,
           generalPakName);
    return 0;
  }
  size_t size = _envir.pakGeneral.entries[index].fileSize;
  if (size) {
    file->buffer = PakFileGetEntryData(&_envir.pakGeneral, index);
    file->bufferSize = _envir.pakGeneral.entries[index].fileSize;
    return 1;
  }
  return 0;
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

int GameEnvironmentGetFileWithExt(GameFile *file, const char *name,
                                  const char *ext) {
  assert(file);
  assert(name);
  assert(ext);
  const size_t fullNameSize = strlen(name) + sizeof(ext) + 2; // dot and null
  char *fullName = malloc(fullNameSize);
  assert(fullName);
  assert(snprintf(fullName, fullNameSize, "%s.%s", name, ext) < fullNameSize);
  int ret = GameEnvironmentGetFile(file, fullName);
  free(fullName);
  return ret;
}

int GameEnvironmentGetLangFile(GameFile *file, const char *name) {
  assert(file);
  assert(name);
  return GameEnvironmentGetFileWithExt(file, name,
                                       LanguageGetExtension(_envir.lang));
}
