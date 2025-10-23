#include "game_envir.h"
#include "formats/format_lang.h"
#include "logger.h"
#include "pak_file.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *pakFiles[] = {
    "CATWALK.PAK", "CAVE1.PAK", "CIMMERIA.PAK", "DRIVERS.PAK", "FOREST1.PAK",
    "GENERAL.PAK", "KEEP.PAK",  "L01.PAK",      "L02.PAK",     "L03.PAK",
    "L04.PAK",     "L05.PAK",   "L06.PAK",      "L07.PAK",     "L08.PAK",
    "L09.PAK",     "L10.PAK",   "L11.PAK",      "L12.PAK",     "L13.PAK",
    "L14.PAK",     "L15.PAK",   "L16.PAK",      "L17.PAK",     "L18.PAK",
    "L19.PAK",     "L20.PAK",   "L21.PAK",      "L22.PAK",     "L23.PAK",
    "L24.PAK",     "L25.PAK",   "L26.PAK",      "L27.PAK",     "L28.PAK",
    "L29.PAK",     "MANOR.PAK", "MINE1.PAK",    "MONSTER.PAK", "MUSIC.PAK",
    "O00A.PAK",    "O01A.PAK",  "O01B.PAK",     "O01C.PAK",    "O01D.PAK",
    "O01E.PAK",    "O02A.PAK",  "O02B.PAK",     "O03A.PAK",    "O03B.PAK",
    "O03C.PAK",    "O04A.PAK",  "O07A.PAK",     "O08A.PAK",    "O10A.PAK",
    "O11A.PAK",    "O12A.PAK",  "O14A.PAK",     "O16A.PAK",    "O17A.PAK",
    "O18A.PAK",    "O19A.PAK",  "O21A.PAK",     "O22A.PAK",    "O22B.PAK",
    "O22C.PAK",    "O22D.PAK",  "O23A.PAK",     "O23B.PAK",    "O24A.PAK",
    "O26A.PAK",    "O27A.PAK",  "O28A.PAK",     "O29A.PAK",    "RUIN.PAK",
    "STARTUP.PAK", "SWAMP.PAK", "TOWER1.PAK",   "URBISH.PAK",  "VOC.PAK",
    "YVEL.PAK",    NULL,
};

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

int GameEnvironmentInit(const char *dataDir, Language lang) {
  assert(dataDir);
  memset(&_envir, 0, sizeof(GameEnvironment));
  _envir.dataDir = strdup(dataDir);
  _envir.lang = lang;
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

static char *genNameWithExt(const char *name, const char *ext) {
  const size_t fullNameSize = strlen(name) + sizeof(ext) + 2; // dot and null
  char *fullName = malloc(fullNameSize);
  assert(fullName);
  assert(snprintf(fullName, fullNameSize, "%s.%s", name, ext) < fullNameSize);
  return fullName;
}

static char *strtoupper(char *dest, const char *src) {
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
    Log("GAME_ENVIR", "get file %s", name);
    file->buffer = PakFileGetEntryData(pak, index);
    file->bufferSize = PakFileGetEntrySize(pak, index);
    return 1;
  }
  return 0;
}

int GameEnvironmentGetStartupFileWithExt(GameFile *file, const char *name,
                                         const char *ext) {
  assert(file);
  assert(name);
  assert(ext);
  char *fullName = genNameWithExt(name, ext);
  assert(fullName);
  int ret = GameEnvironmentGetStartupFile(file, fullName);
  free(fullName);
  return ret;
}

int GameEnvironmentGetStartupFile(GameFile *file, const char *name) {
  return getFile(&_envir.pakStartup, file, name);
}

int GameEnvironmentGetGeneralFile(GameFile *file, const char *name) {
  return getFile(&_envir.pakGeneral, file, name);
}

int GameEnvironmentLoadPak(const char *pakFileName) {
  int cacheIndex = GetCacheIndex(pakFileName);
  if (cacheIndex != -1) {
    return 1;
  }

  const size_t fullPathSize = strlen(_envir.dataDir) + strlen(pakFileName) + 2;
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
  AddInCache(&f, pakFileName);
  return 1;
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
  int pakIndex = GameEnvironmentFindPak(name);
  if (pakIndex != -1) {
    GameEnvironmentLoadPak(pakFiles[pakIndex]);
    return GameEnvironmentGetFile(file, name);
  }
  return 0;
}

int GameEnvironmentFindPak(const char *filename) {
  int i = 0;
  const char *pakFile = pakFiles[0];
  while (pakFile != NULL) {
    PAKFile f;
    PAKFileInit(&f);
    char *fullP = resolvePakName(pakFile);
    PAKFileRead(&f, fullP);
    free(fullP);
    int index = PakFileGetEntryIndex(&f, filename);
    PAKFileRelease(&f);
    if (index != -1) {
      return i;
    }
    i++;
    pakFile = pakFiles[i];
  }
  return -1;
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
  Log("GAME_ENVIR", "get file %s from pak %s", filename, pakFileName);
  file->buffer = PakFileGetEntryData(pak, fIndex);
  file->bufferSize = PakFileGetEntrySize(pak, fIndex);
  return 1;
}
