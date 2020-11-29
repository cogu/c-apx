#ifndef APX_LOGGING_H
#define APX_LOGGING_H

/**
* temporary placeholder for APX logging functionality, replace later with system log, or file log support
*/
extern int8_t g_debug; // Global variable from main

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
# define APX_LOG_DEBUG(fmt, ...)
# define APX_LOG_INFO(fmt, ...)
# define APX_LOG_WARNING(fmt, ...)
# define APX_LOG_ERROR(fmt, ...)
#else
# include <stdio.h>
# define APX_LOG_DEBUG(fmt, ...) if(g_debug != 0){fprintf(stdout, fmt "\n", ##__VA_ARGS__);}
# define APX_LOG_INFO(fmt, ...) if(g_debug != 0){fprintf(stdout, fmt "\n", ##__VA_ARGS__);}
# define APX_LOG_WARNING(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
# define APX_LOG_ERROR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_LOGGING_H
