#include "Logger_a.h"

log_t *__static_logger_ptr = NULL;

static int CreateLogName(log_t *log)
{
        struct tm tTemp = {0};
        char fileName[512] =  {0};
        time_t tNow = time(NULL);

        localtime_r(&tNow, &tTemp);
        snprintf(fileName, sizeof(fileName), "%s/%s%04d%02d%02d%02d%02d%02d.log",
                        log->logPath, log->logName, tTemp.tm_year + 1900, tTemp.tm_mon + 1, tTemp.tm_mday,
                        tTemp.tm_hour, tTemp.tm_min, tTemp.tm_sec);

        if (access(fileName, F_OK) != 0) {
                close(log->fd);
                log->fd = open(fileName, O_WRONLY|O_CREAT|O_NOCTTY|O_APPEND, 0666);
                if (log->fd == -1 ) {
                        fprintf(stderr, "log_open: Opening logfile %s: %s",
                                        fileName, strerror(errno));
                        return -1;
                }
        }


        return 0;
}


/* Allows printf()-like interface to file descriptors without the
 * complications that arise from mixing stdio and low level calls 
 * FIXME: Needs date and time before logfile entries.
 */
int lprintf( log_t *log, unsigned int level, char *fmt, ... ) {
        int rc = -1;
        int cnt = 0;
        int linecnt = 0;
        //char date[32] = {0};
        //char threadnum[10] = {0};
        char line[LOGLINE_MAX] = {0};
        struct tm tTemp = {0};
        struct timeval tvTemp = {0};
        va_list ap;
        static const char *levels[6] = { "[TRACE] ", 
                "[DEBUG] ", 
                "[INFO] ", 
                "[WARN] ", 
                "[ERROR] ", 
                "[FATAL] "};

        if(!log) return -1;

        /*
        //If this is debug info, and we're not logging it, return
        if( !(log->flags&LOG_DEBUG) && level == DEBUG ) return 0;

        // Prepare the date string
        if( !(log->flags&LOG_NODATE) ) {
        now=time(NULL);
        strcpy(date,ctime(&now));
        date[strlen(date)-6]=' ';
        date[strlen(date)-5]='\0';
        }

        // thread num
        if( !(log->flags&LOG_NOTID) ) {
        sprintf(threadnum, "(%lu) ", pthread_self());
        }

        //Format layout
        cnt = snprintf(line, sizeof(line), "%s%s%s",
        log->flags&LOG_NODATE ? "" : date,
        log->flags&LOG_NOLVL  ? "" : 
        (level > FATAL ? levels[0] : levels[level]),
        log->flags&LOG_NOTID  ? "" : threadnum);
        */

        /* format datetime + level */
        gettimeofday(&tvTemp, NULL);
        localtime_r(&tvTemp.tv_sec, &tTemp);

        if (log->flags&LOG_TID) {
                cnt = sprintf(line, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld][%lu]%s", tTemp.tm_year + 1900,
                                tTemp.tm_mon + 1, tTemp.tm_mday, tTemp.tm_hour, tTemp.tm_min, tTemp.tm_sec,
                                tvTemp.tv_usec/1000, pthread_self(), levels[level]);
        } else {
                cnt = sprintf(line, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld]%s", tTemp.tm_year + 1900,
                                tTemp.tm_mon + 1, tTemp.tm_mday, tTemp.tm_hour, tTemp.tm_min, tTemp.tm_sec,
                                tvTemp.tv_usec/1000, levels[level]);
        }

        va_start(ap, fmt);
        linecnt = vsnprintf(line+cnt, sizeof(line)-cnt, fmt, ap);    /*如果输入的日志过长会自动截取*/
        va_end(ap);

        //line[sizeof(line)-1] = '\0';
        /*
        if( !(log->flags&LOG_NOLF) ) {
        //chomp(line);
        //strcpy(line+strlen(line), "\n");
        }
        */

        /* check date or size */
        if (log->flags&LOG_DAILY) {
                sem_wait(&log->sem);         
                if (log->cur_day != tTemp.tm_mday) {
                        if (CreateLogName(log) < 0) 
                                return -1;
                        log->cur_day = tTemp.tm_mday;
                }
                rc = write(log->fd, line, linecnt + cnt);
                sem_post(&log->sem);
        } else {
                sem_wait(&log->sem);         
                log->cur_size = log->cur_size + linecnt + cnt;
                if (log->cur_size > log->max_size) {
                        if (CreateLogName(log) < 0)
                                return -1;
                        log->cur_size = linecnt + cnt;
                }
                rc = write(log->fd, line, linecnt + cnt);
                sem_post(&log->sem);
        }

        if( !rc ) errno = 0;
        return rc;
}

log_t *log_open(char *pathname, char *logname, int flags, unsigned int size) {
        struct tm tTemp = {0};
        char fileName[512] = {0};

        log_t *log = (log_t *)malloc(sizeof(log_t));
        if (!log) {
                fprintf(stderr, "log_open: Unable to malloc()");
                return NULL;
        }

        strcpy(log->logPath, pathname);
        if (logname == NULL) {
                strcpy(log->logName, "AppInfo");
        } else {
                strcpy(log->logName, logname);
        }

        time_t tNow = time(NULL);
        localtime_r(&tNow, &tTemp);
        snprintf(fileName, sizeof(fileName), "%s/%s%04d%02d%02d%02d%02d%02d.log",
                        log->logPath, log->logName, tTemp.tm_year + 1900, tTemp.tm_mon + 1, tTemp.tm_mday,
                        tTemp.tm_hour, tTemp.tm_min, tTemp.tm_sec);

        log->cur_size = 0;
        log->flags = flags;
        if (size == 0) 
                log->max_size = 1024 * 1024 * 1024;
        else 
                log->max_size = size;
        log->cur_day = tTemp.tm_mday;


        if (!strcmp(logname,"-") ) {
                log->fd = 2;
        } else {
                log->fd = open(fileName, O_WRONLY|O_CREAT|O_NOCTTY|O_APPEND, 0666);
        }
        if (log->fd == -1 ) {
                fprintf(stderr, "log_open: Opening logfile %s: %s", 
                                fileName, strerror(errno));
                free(log);
                return NULL;
        }

        if (sem_init(&log->sem, 0, 1) == -1 ) {
                fprintf(stderr, "log_open: Could not initialize log semaphore.");
                close(log->fd);
                free(log);
                return NULL;
        }
        __static_logger_ptr = log;

        return log;
}

int log_close(log_t *log) {
        if (log != NULL) {
                sem_wait(&log->sem);
                sem_destroy(&log->sem);
                close(log->fd);
                free(log);
                __static_logger_ptr = NULL;
        }

        return 0;
}
