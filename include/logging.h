#ifndef LOGGING_H
#define LOGGING_H 1

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern int log_enabled;
extern FILE *log_file;

#ifdef _WIN64
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

// change this to change the log level
#define LOG_LEVEL 4
#define USE_FILENAMES 1

#if (USE_FILENAMES == 1)
#define __FILENAME__ (strrchr(__FILE__, SEPARATOR) ? strrchr(__FILE__, SEPARATOR) + 1 : __FILE__)
#else
#define __FILENAME__ ""
#endif
// use these to log stuff

#if (LOG_LEVEL >= 1)
#define LOG_ERROR(format, ...) log_msg("%s %s: %d " format, "ERROR", __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_ERROR(format, ...)
#endif

#if (LOG_LEVEL >= 2)
#define LOG_WARNING(format, ...) log_msg("%s: %s:%d " format, "WARNING", __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_WARNING(format, ...)
#endif

#if (LOG_LEVEL >= 3)
#define LOG_INFO(format, ...) log_msg("%s: %s:%d " format, "INFO", __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(format, ...)
#endif

#if (LOG_LEVEL >= 4)
#define LOG_DEBUG(format, ...) log_msg("%s: %s:%d " format, "DEBUG", __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

void log_start(const char *fname);
void log_end();
void log_msg(const char *format, ...);

#endif