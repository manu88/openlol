#pragma once
#include <stddef.h>
#include <stdint.h>
/*
This data type is *not* a Westwood/Lands of Lore original format!
this is a simple 1-line key=val entry system stored in a text file
*/

typedef struct {
  const char *key;
  const char *val;
} ConfigEntry;

typedef struct {
  ConfigEntry *entries;
  size_t numEntries;
} ConfigHandle;

int ConfigHandleFromFile(ConfigHandle *handle, const char *filepath);
int ConfigHandleWriteFile(const ConfigHandle *handle, const char *filepath);
void ConfigHandleRelease(ConfigHandle *handle);
const char *ConfigHandleGetValue(const ConfigHandle *handle, const char *key);
float ConfigHandleGetValueFloat(const ConfigHandle *handle, const char *key,
                                float defaultVal);
int ConfigHandleSetValue(ConfigHandle *handle, const char *key,
                         const char *val);
int ConfigHandleSetValueInt(ConfigHandle *handle, const char *key,
                         int val);
