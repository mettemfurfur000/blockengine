#include "../include/logging.h"

int log_enabled = 0;
FILE *log_file = NULL;

void log_start(const char *fname)
{
    log_file = fopen(fname, "w");

    if (!log_file)
        printf("Failed to open log file: %s\n", fname);
    else
        printf("Logging enabled.\n");
    log_enabled = !!log_file;
}

void log_end()
{
    if (!log_enabled)
        return;

    fclose(log_file);
    log_file = NULL;
    log_enabled = 0;
}

void log_msg(const char *format, ...)
{
    if (!log_enabled)
        return;

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fprintf(log_file, "\n");

    fflush(log_file);

    return;
}