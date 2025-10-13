#include "game_envir.h"
#include "formats/format_lang.h"
#include "pak_file.h"
#include <_string.h>
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  PAKFile file;
  char *name;
} PakFileCache;

typedef struct {
  const char *dataDir;
  Language lang;

  PAKFile *currentLevelPak;

  PAKFile pakGeneral;
  PAKFile pakStartup;

  PakFileCache *cache;
  int cacheIndex;
  int cacheSize;
} GameEnvironment;

static GameEnvironment _envir;

#define CACHE_SIZE_INCREMENT 10
static int GetCacheIndex(const char *name) {
  for (int i = 0; i < _envir.cacheIndex; i++) {
    if (strcmp(name, _envir.cache[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

static int AddInCache(PAKFile *f, const char *pakFileName) {
  if (_envir.cacheIndex >= _envir.cacheSize) {
    _envir.cacheSize += CACHE_SIZE_INCREMENT;
    _envir.cache =
        realloc(_envir.cache, _envir.cacheSize * sizeof(PakFileCache));
    assert(_envir.cache);
  }
  _envir.cache[_envir.cacheIndex].file = *f;
  _envir.cache[_envir.cacheIndex].name = strdup(pakFileName);
  return _envir.cacheIndex++;
}

static const char generalPakName[] = "GENERAL.PAK";
static const char startupPakName[] = "STARTUP.PAK";

static char *resolvePakName(const char *name) {
  size_t s = strlen(_envir.dataDir) + 2 + strlen(name);
  char *path = malloc(s);
  assert(path);
  snprintf(path, s, "%s/%s", _envir.dataDir, name);
  return path;
}

static int loadPak(PAKFile *f, const char *pakfile) {
  char *path = resolvePakName(pakfile);
  assert(PAKFileRead(f, path));
  free(path);
  return 1;
}

int GameEnvironmentInit(const char *dataDir) {
  assert(dataDir);
  memset(&_envir, 0, sizeof(GameEnvironment));
  _envir.dataDir = strdup(dataDir);
  _envir.lang = Language_EN;
  printf("GameEnvironmentInit dataDir='%s' lang=%s\n", dataDir,
         LanguageGetExtension(_envir.lang));

  PAKFileInit(&_envir.pakGeneral);
  loadPak(&_envir.pakGeneral, generalPakName);

  PAKFileInit(&_envir.pakStartup);
  loadPak(&_envir.pakStartup, startupPakName);

  _envir.cache = malloc(sizeof(PakFileCache) * CACHE_SIZE_INCREMENT);
  _envir.cacheSize = CACHE_SIZE_INCREMENT;
  return 1;
}

int GameEnvironmentLoadLevel(uint8_t index) {
  assert(index < 9);
  const size_t fullPathSize =
      strlen(_envir.dataDir) +
      14; // / sizeof('CHAPTER') + INDEX sizeof('.PAK') + NULL
  char *fullPath = malloc(fullPathSize);
  assert(fullPath);
  assert(snprintf(fullPath, fullPathSize, "%s/L%02i.PAK", _envir.dataDir,
                  index) < fullPathSize);
  printf("GameEnvironmentLoadLevel load level %i '%s'\n", index, fullPath);

  PAKFile f;
  PAKFileInit(&f);
  int ret = PAKFileRead(&f, fullPath);

  if (ret) {
    int cacheIndex = AddInCache(&f, fullPath + strlen(_envir.dataDir) + 1);
    _envir.currentLevelPak = &_envir.cache[cacheIndex].file;
  }
  free(fullPath);
  return ret;
}

void GameEnvironmentRelease(void) {
  free((void *)_envir.dataDir);
  PAKFileRelease(&_envir.pakGeneral);
  PAKFileRelease(&_envir.pakStartup);

  for (int i = 0; i < _envir.cacheIndex; i++) {
    PAKFileRelease(&_envir.cache[i].file);
    free(_envir.cache[i].name);
  }
  free(_envir.cache);
}

char *strtoupper(char *dest, const char *src) {
  char *result = dest;
  while ((*dest++ = toupper(*src++)))
    ;
  return result;
}

static int getFile(PAKFile *pak, GameFile *file, const char *name) {
  int index = PakFileGetEntryIndex(pak, name);
  if (index == -1) {
    // try with upper name
    char *upperName = strdup(name);
    strtoupper(upperName, name);
    if (strcmp(upperName, name) == 0) {
      free(upperName);
      return 0;
    }
    int ret = getFile(pak, file, upperName);
    free(upperName);
    return ret;
  }
  size_t size = pak->entries[index].fileSize;
  if (size) {
    file->buffer = PakFileGetEntryData(pak, index);
    file->bufferSize = PakFileGetEntrySize(pak, index);
    return 1;
  }
  return 0;
}
int GameEnvironmentGetStartupFile(GameFile *file, const char *name) {
  return getFile(&_envir.pakStartup, file, name);
}

int GameEnvironmentGetGeneralFile(GameFile *file, const char *name) {
  return getFile(&_envir.pakGeneral, file, name);
}

int GameEnvironmentGetGeneralLangFile(GameFile *file) {
  return GameEnvironmentGetGeneralFileWithExt(
      file, "LANDS", LanguageGetExtension(_envir.lang));
}

int GameEnvironmentGetFile(GameFile *file, const char *name) {
  assert(file);
  assert(name);
  if (_envir.currentLevelPak) {
    if (getFile(_envir.currentLevelPak, file, name)) {
      return 1;
    }
  }
  for (int i = 0; i < _envir.cacheIndex; i++) {
    if (_envir.currentLevelPak == &_envir.cache[i].file) {
      continue;
    }
    if (getFile(&_envir.cache[i].file, file, name)) {
      return 1;
    }
  }
  if (getFile(&_envir.pakGeneral, file, name)) {
    return 1;
  }
  if (getFile(&_envir.pakStartup, file, name)) {
    return 1;
  }
  return 0;
}

static char *genNameWithExt(const char *name, const char *ext) {
  const size_t fullNameSize = strlen(name) + sizeof(ext) + 2; // dot and null
  char *fullName = malloc(fullNameSize);
  assert(fullName);
  assert(snprintf(fullName, fullNameSize, "%s.%s", name, ext) < fullNameSize);
  return fullName;
}

int GameEnvironmentGetFileWithExt(GameFile *file, const char *name,
                                  const char *ext) {
  assert(file);
  assert(name);
  assert(ext);
  char *fullName = genNameWithExt(name, ext);
  assert(fullName);
  int ret = GameEnvironmentGetFile(file, fullName);
  free(fullName);
  return ret;
}

int GameEnvironmentGetGeneralFileWithExt(GameFile *file, const char *name,
                                         const char *ext) {
  assert(file);
  assert(name);
  assert(ext);
  char *fullName = genNameWithExt(name, ext);
  assert(fullName);
  int ret = GameEnvironmentGetGeneralFile(file, fullName);
  free(fullName);
  return ret;
}

int GameEnvironmentGetLangFile(GameFile *file, const char *name) {
  assert(file);
  assert(name);
  return GameEnvironmentGetFileWithExt(file, name,
                                       LanguageGetExtension(_envir.lang));
}

int GameEnvironmentGetFileFromPak(GameFile *file, const char *filename,
                                  const char *pakFileName) {
  PAKFile *pak = NULL;
  int cacheIndex = GetCacheIndex(pakFileName);
  if (cacheIndex != -1) {
    pak = &_envir.cache[cacheIndex].file;
  } else {
    const size_t fullPathSize =
        strlen(_envir.dataDir) + strlen(pakFileName) + 2;
    char *fullPath = malloc(fullPathSize);
    assert(fullPath);
    assert(snprintf(fullPath, fullPathSize, "%s/%s", _envir.dataDir,
                    pakFileName) < fullPathSize);

    PAKFile f = {0};
    PAKFileInit(&f);
    if (PAKFileRead(&f, fullPath) == 0) {
      free(fullPath);
      return 0;
    }
    free(fullPath);
    int cacheIndex = AddInCache(&f, pakFileName);
    pak = &_envir.cache[cacheIndex].file;
  }
  assert(pak);
  int fIndex = PakFileGetEntryIndex(pak, filename);
  if (fIndex == -1) {
    return 0;
  }
  file->buffer = PakFileGetEntryData(pak, fIndex);
  file->bufferSize = PakFileGetEntrySize(pak, fIndex);
  return 1;
}
