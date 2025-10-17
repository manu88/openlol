#include "logger.h"
#include <stdio.h>
#include <string.h>

static void stdoutLogFunc(const char *prefix, const char *fmt, va_list args) {
  printf("[%s] ", prefix);
  vprintf(fmt, args);
  printf("\n");
}

static Logger _stdOutLog = {.func = stdoutLogFunc};

Logger *LoggerStdOut = &_stdOutLog;
static Logger *defaultLogger = NULL;

void Log(const char *prefix, const char *fmt, ...) {
  if (defaultLogger) {
    va_list args;
    va_start(args, fmt);
    defaultLogger->func(prefix, fmt, args);
    va_end(args);
  }
}

void LoggerSetOutput(Logger *log) { defaultLogger = log; }

const Logger *LoggerGetOutput(void) { return defaultLogger; }
