#include "../include/logging.h"
#include <pthread.h>

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

char *log_level_to_str(unsigned char level)
{
    switch (level)
    {
    case 1:
        return "ERROR";
    case 2:
        return "WARNING";
    case 3:
        return "INFO";
    case 4:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

void log_msg(unsigned char level, const char *format, ...)
{
    if (!log_enabled || level == 0)
        return;

    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&log_mutex);

    time_t now = time(NULL);
    char *timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0';
    fprintf(log_file, "[%s] [%s] ", timestr, log_level_to_str(level));

    va_list args;
    va_start(args, format);
    int ret = vfprintf(log_file, format, args);
    va_end(args);

    if (ret < 0)
    {
        // Handle error
    }

    fprintf(log_file, "\n");
    fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}