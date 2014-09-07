#ifndef __CHARCOAL_LOGGER_H__
#define __CHARCOAL_LOGGER_H__


#define    LOG_TRACE 1
#define    LOG_DEBUG 2
#define    LOG_INFO  3
#define    LOG_WARN  4
#define    LOG_ERROR 5
#define    LOG_FATAL 6

typedef struct logger_t logger_t;

logger_t * logger_new(char *name, int level);
void logger_destroy(logger_t *logger);
void logger_log(int level, const char* format, ...);

#endif