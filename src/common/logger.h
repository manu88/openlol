#pragma once
#include <stdarg.h>

#define PRINTFLIKE(n, m) __attribute__((format(printf, n, m)))

typedef void (*LOGFunction)(const char *prefix, const char *fmt, va_list args)
    PRINTFLIKE(3, 0);

typedef struct _Logger {
  LOGFunction func;
  int level;
} Logger;

extern Logger *LoggerStdOut;

void LoggerSetOutput(Logger *log);
const Logger *LoggerGetOutput(void);
void Log(const char *prefix, const char *fmt, ...) PRINTFLIKE(2, 3);
void LogEnablePrefix(const char *prefix);
void LogDisablePrefix(const char *prefix);
