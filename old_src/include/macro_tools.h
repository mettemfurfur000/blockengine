#ifndef MACRO_TOOLS_H
#define MACRO_TOOLS_H

#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DEBUG_MESSAGES 1

#if DEBUG_MESSAGES == 1
#define DEBUG_REPORT_ERROR(error_string, function, value) fprintf(stderr, "[ERROR REPORT]\t%s\t%s\t%s\t:%s - %s\n", \
	__FILENAME__, __LINE__, #function, #error_string #value);
#else
#define DEBUG_REPORT_ERROR(error_string)
#endif

#endif