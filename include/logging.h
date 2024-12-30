#ifndef LOGGING_H
#define LOGGING_H 1

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern int log_enabled;
extern FILE *log_file;

#define LOG_LEVEL 4

// use these to log stuff

#if (LOG_LEVEL >= 1)
#define LOG_ERROR(format, ...) log_msg("ERROR: " format, ##__VA_ARGS__);
#else
#define LOG_ERROR(format, ...) ;
#endif

#if (LOG_LEVEL >= 2)
#define LOG_WARNING(format, ...) log_msg("WARNING: " format, ##__VA_ARGS__);
#else
#define LOG_WARNING(format, ...) ;
#endif

#if (LOG_LEVEL >= 3)
#define LOG_INFO(format, ...) log_msg("INFO: " format, ##__VA_ARGS__);
#else
#define LOG_INFO(format, ...) ;
#endif

#if (LOG_LEVEL >= 4)
#define LOG_DEBUG(format, ...) log_msg("DEBUG: " format, ##__VA_ARGS__);
#else
#define LOG_DEBUG(format, ...) ;
#endif

void log_start(const char *fname);
void log_end();
void log_msg(const char *format, ...);

#endif
