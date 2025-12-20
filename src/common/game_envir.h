#pragma once
#include "formats/format_lang.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
How to find in which pak is located a given file?
- First of all, all file names are unique so you won't find a file named 'foo'
in two different pak files.
- Second, per-level files are located in level-related pak files. For example
files like 'LEVEL1.SHP', 'LEVEL1.INF' are located in 'L01.PAK'.
- Lastly, some general files like UI assets or item icons are located in
general pak files like 'GENERAL.PAK' and 'STARTUP.PAK'.

So here's the heuristic when loading a file;
- If the file load request comes from a script - eg LoadGraphics then it should
be straightforward to deduce the PAK file from the file path. Example:
'LEVEL1.INF' is in LO1.PAK.

- If the file needs to be loaded from main code, either specify the PAK from which
to load the content, or load pak files one by one until the right file is found. This
last option is doable because all file names are unique, but it will be
slooooooooow.
*/
typedef struct {
  uint8_t *buffer;
  size_t bufferSize;
} GameFile;

int GameEnvironmentInit(const char *dataDir, Language lang);
void GameEnvironmentRelease(void);

int GameEnvironmentLoadLevel(uint8_t index);

int GameEnvironmentGetFile(GameFile *file, const char *name);
int GameEnvironmentFindPak(const char *filename);

int GameEnvironmentGetGeneralFile(GameFile *file, const char *name);
int GameEnvironmentGetStartupFile(GameFile *file, const char *name);
int GameEnvironmentGetStartupFileWithExt(GameFile *file, const char *name,
                                         const char *ext);

int GameEnvironmentGetFileFromPak(GameFile *file, const char *filename,
                                  const char *pakFile);

int GameEnvironmentGetFileWithExt(GameFile *file, const char *name,
                                  const char *ext);

int GameEnvironmentGetLangFile(GameFile *file, const char *name);
