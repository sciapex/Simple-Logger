#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOGLINE_MAX 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        int fd;
        int flags;

        int cur_day;
        unsigned int cur_size;
        unsigned int max_size;

        char logPath[512];
        char logName[64];

        sem_t sem;
} log_t;

#ifndef __STATIC_LOGGER_PTR
#define __STATIC_LOGGER_PTR
        extern log_t *__static_logger_ptr;
#endif

#define LEVEL_TRACE 0
#define LEVEL_DEBUG 1
#define LEVEL_INFO  2
#define LEVEL_WARN  3
#define LEVEL_ERROR 4
#define LEVEL_FATAL 5

#define LOG_APPEND  1<<0
#define LOG_SIZE    1<<1
#define LOG_DAILY   1<<2
#define LOG_DEBUG   1<<3
#define LOG_STDERR  1<<4
#define LOG_TID     1<<5

#define LOG_NOLF    1<<6
#define LOG_NODATE  1<<7
#define LOG_NOLVL   1<<8

#define DEBUG_S(fmt, args...) lprintf(__static_logger_ptr, LEVEL_DEBUG, fmt, args);
#define INFO_S(fmt, args...)  lprintf(__static_logger_ptr, LEVEL_INFO, fmt, args);
#define WARN_S(fmt, args...)  lprintf(__static_logger_ptr, LEVEL_WARN, fmt, args);
#define ERROR_S(fmt, args...) lprintf(__static_logger_ptr, LEVEL_ERROR, fmt, args);
#define FATAL_S(fmt, args...) lprintf(__static_logger_ptr, LEVEL_FATAL, fmt, args);

#define DEBUG(fmt, ...) lprintf(__static_logger_ptr, LEVEL_DEBUG, fmt, ##__VA_ARGS__);
#define INFO(fmt, ...)  lprintf(__static_logger_ptr, LEVEL_INFO, fmt, ##__VA_ARGS__);
#define WARN(fmt, ...)  lprintf(__static_logger_ptr, LEVEL_WARN, fmt, ##__VA_ARGS__);
#define ERROR(fmt, ...) lprintf(__static_logger_ptr, LEVEL_ERROR, fmt, ##__VA_ARGS__);
#define FATAL(fmt, ...) lprintf(__static_logger_ptr, LEVEL_FATAL, fmt, ##__VA_ARGS__);
/*
 * Logs to the logfile using printf()-like format strings.
 * 
 * log_t - The value you got back from a log_open() call.
 * level - can be one of: DEBUG, INFO, WARN, ERROR, FATAL
 * fmt   - is a printf()-like format string, followed by the parameters.
 *
 * Returns 0 on success, or -1 on failure.
 */
int lprintf(log_t *log, unsigned int level, char *fmt, ... );

/*
 * Initializes the logfile to be written to with fprintf().
 *
 * fname - The name of the logfile to write to
 * flags - The bitwise 'or' of zero or more of the following flags:
 *          LOG_TRUNC   - Truncates the logfile on opening
 *          LOG_NODATE  - Omits the date from each line of the log
 *          LOG_NOLF    - Keeps from inserting a trailing '\n' when you don't.
 *          LOG_NOLVL   - Keeps from inserting a log level indicator.
 *          LOG_STDERR  - Sends log data to stderr as well as to the log.
 *                        (this not implemented yet)
 *
 * Returns NULL on failure, and a valid log_t (value > 0) on success.
 */
log_t *log_open( char *pname, char *fname, int flags = LOG_SIZE, unsigned int size = 0);

/*
 * Closes a logfile when it's no longer needed
 * 
 * log  - The log_t corresponding to the log you want to close
 */
int log_close();

#ifdef __cplusplus
}
#endif

#endif
