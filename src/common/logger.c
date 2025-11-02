#include "logger.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void stdoutLogFunc(const char *prefix, const char *fmt, va_list args) {
  printf("[%s] ", prefix);
  vprintf(fmt, args);
  printf("\n");
}

static Logger _stdOutLog = {.func = stdoutLogFunc};

Logger *LoggerStdOut = &_stdOutLog;

typedef struct {
  Logger *defaultLogger;

  char **prefixes;
  size_t numPrefixes;
} LogSystem;

static LogSystem _logSystem = {0};

static int prefixIndex(const char *prefix) {
  for (int i = 0; i < _logSystem.numPrefixes; i++) {
    if (_logSystem.prefixes[i] && strcmp(prefix, _logSystem.prefixes[i]) == 0) {
      return i;
    }
  }
  return -1;
}

static int filterPrefix(const char *prefix) {
  if (_logSystem.prefixes == NULL) {
    return 1;
  }
  return prefixIndex(prefix) != -1;
}

static void addPrefix(const char *prefix) {
  for (int i = 0; i < _logSystem.numPrefixes; i++) {
    if (_logSystem.prefixes[i] == NULL) {
      _logSystem.prefixes[i] = strdup(prefix);
      return;
    }
  }
  size_t prevSize = _logSystem.numPrefixes * sizeof(char *);
  _logSystem.numPrefixes *= 2;
  size_t newSize = _logSystem.numPrefixes * sizeof(char *);
  _logSystem.prefixes = realloc(_logSystem.prefixes, newSize);
  memset(_logSystem.prefixes + prevSize, 0, newSize - prevSize);
  addPrefix(prefix);
}

void Log(const char *prefix, const char *fmt, ...) {
  if (_logSystem.defaultLogger && filterPrefix(prefix)) {
    va_list args;
    va_start(args, fmt);
    _logSystem.defaultLogger->func(prefix, fmt, args);
    va_end(args);
  }
}

void LoggerSetOutput(Logger *log) { _logSystem.defaultLogger = log; }

const Logger *LoggerGetOutput(void) { return _logSystem.defaultLogger; }

void LogEnablePrefix(const char *prefix) {
  if (_logSystem.prefixes == NULL) {
    _logSystem.numPrefixes = 10;
    size_t newSize = _logSystem.numPrefixes * sizeof(char *);
    _logSystem.prefixes = malloc(newSize);
    memset(_logSystem.prefixes, 0, newSize);
  }
  if (prefixIndex(prefix) != -1) {
    return;
  }

  addPrefix(prefix);
}

void LogDisablePrefix(const char *prefix) {
  for (int i = 0; i < _logSystem.numPrefixes; i++) {
    if (strcmp(prefix, _logSystem.prefixes[i]) == 0) {
      free(_logSystem.prefixes[i]);
      _logSystem.prefixes[i] = NULL;
      return;
    }
  }
  return;
}
