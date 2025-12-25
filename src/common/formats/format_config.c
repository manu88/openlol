#include "format_config.h"
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trimLeft(char *str) {
  while (isspace((unsigned char)*str))
    str++;
  return str;
}

char *substr(const char *s, int x, int y) {
  char *ret = malloc(strlen(s) + 1);
  char *p = ret;
  const char *q = &s[x];

  assert(ret != NULL);

  while (x < y) {
    *p++ = *q++;
    x++;
  }

  *p++ = '\0';

  return ret;
}

static ConfigEntry *getEntry(const ConfigHandle *handle, const char *key) {
  for (int i = 0; i < handle->numEntries; i++) {
    if (strcmp(handle->entries[i].key, key) == 0) {
      return handle->entries + i;
    }
  }
  return NULL;
}

int ConfigHandleSetValue(ConfigHandle *handle, const char *key,
                         const char *val) {
  ConfigEntry *entry = getEntry(handle, key);
  if (entry) {
    free((void *)entry->val);
    entry->val = strdup(val);
    return 1;
  }

  handle->numEntries++;
  handle->entries =
      realloc(handle->entries, sizeof(ConfigEntry) * handle->numEntries);
  assert(handle->entries);
  entry = handle->entries + handle->numEntries - 1;
  entry->key = strdup(key);
  entry->val = strdup(val);
  return 1;
}

int ConfigHandleSetValueInt(ConfigHandle *handle, const char *key, int val) {
  char str[10] = "";
  snprintf(str, 10, "%i", val);
  return ConfigHandleSetValue(handle, key, str);
}

static int parseLineEntry(ConfigHandle *handle, const char *line) {
  const char *sep = strchr(line, '=');
  if (sep == NULL) {
    return 0;
  }

  const char *key = substr(line, 0, sep - line);
  if (getEntry(handle, key) != NULL) {
    printf("duplicate key '%s'\n", key);
    free((void *)key);
    return 0;
  }
  handle->numEntries++;
  handle->entries =
      realloc(handle->entries, sizeof(ConfigEntry) * handle->numEntries);
  assert(handle->entries);
  ConfigEntry *entry = handle->entries + handle->numEntries - 1;
  entry->key = key;
  entry->val = strdup(sep + 1);

  return 1;
}

int ConfigHandleWriteFile(const ConfigHandle *handle, const char *filepath) {
  FILE *fptr = fopen(filepath, "w");
  if (fptr == NULL) {
    return 0;
  }
  for (int i = 0; i < handle->numEntries; i++) {
    fprintf(fptr, "%s=%s\n", handle->entries[i].key, handle->entries[i].val);
  }
  fclose(fptr);

  return 1;
}

int ConfigHandleFromFile(ConfigHandle *handle, const char *filepath) {
  memset(handle, 0, sizeof(ConfigHandle));
  FILE *fptr = fopen(filepath, "r");
  if (!fptr) {
    return 0;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int lineNum = 0;

  int error = 0;
  while ((read = getline(&line, &len, fptr)) != -1) {
    if (read <= 1) {
      continue;
    }
    lineNum++;
    line[read - 1] = 0;
    const char *trimmed = trimLeft(line);
    if (strlen(trimmed) == 0) {
      continue;
    }
    if (trimmed[0] == '#') {
      continue;
    }
    if (!parseLineEntry(handle, trimmed)) {
      printf("Error at line %i '%s'\n", lineNum, line);
      error = 1;
      break;
    }
  }

  fclose(fptr);
  if (line) {
    free(line);
  }
  if (error) {
    ConfigHandleRelease(handle);
    handle->numEntries = 0;
  }
  return !error;
}

float ConfigHandleGetValueFloat(const ConfigHandle *handle, const char *key,
                                float defaultVal) {
  const char *val = ConfigHandleGetValue(handle, key);
  if (!val) {
    return defaultVal;
  }
  return atof(val);
}

const char *ConfigHandleGetValue(const ConfigHandle *handle, const char *key) {
  ConfigEntry *entry = getEntry(handle, key);
  return entry ? entry->val : NULL;
}

void ConfigHandleRelease(ConfigHandle *handle) {
  if (handle->numEntries == 0) {
    return;
  }
  for (int i = 0; i < handle->numEntries; i++) {
    free((void *)handle->entries[i].key);
    free((void *)handle->entries[i].val);
  }
  free(handle->entries);
}
