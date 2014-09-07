
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "../include/charcoal_logger.h"



struct logger_t {
    char* name;
    int level;
};

extern logger_t *g_logger;

logger_t * logger_new(char *name, int level){
    logger_t  *logger = malloc(sizeof(logger_t));
    logger->name = malloc(strlen(name) + 1);
    strcpy(logger->name, name);
    logger->level = level;
    
    return logger;
}

void logger_destroy(logger_t *logger){
    
    if(logger != NULL){
        if(logger->name != NULL){
            free(logger->name);
        }
        
        free(logger);
    }
}

// Formats the string, adds time and name 
char* logger_format(const char* format){
    
    // TODO: Add error handling
    
    time_t t = time(NULL);
    struct tm *current_time = gmtime(&t);
    char c_time_string[20];
    
    if (current_time != NULL)
    {
        if (strftime(c_time_string, sizeof(c_time_string), "%F %T ", current_time)) {
            
        }
    }
    
    size_t new_len = strlen(c_time_string) + strlen(format) + 1;
    
    // string to contain date and content of log
    char* new_format = malloc(sizeof(char) * new_len);
    
    // Clear string
    bzero(new_format, new_len);
    
    strcpy (new_format,c_time_string);
    strcat (new_format,format);
    strcat (new_format,"\n");
    
    return new_format;
}

// Helper function to print out logs
void logger_log(int level, const char* format, ...) {
    
    va_list argptr;
    char* new_format = logger_format(format);
    
    // Read variable argument list
    va_start(argptr, format);
    
    // TODO: make log level configurable
    if(level >= LOG_TRACE){
        // If Error or above also output to stderr
        if(level >= LOG_ERROR){
            vfprintf(stderr, new_format, argptr);
        }
        vfprintf(stdout, new_format, argptr);
        fflush(stdout);
    }
    
    va_end(argptr);
    
    free(new_format);
    
}
