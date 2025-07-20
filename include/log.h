/**
 * To introduce per-level complexity/functionality, uncomment the below functions and 
 * move them to log.c to log messages at different levels. These functions will call the 
 * `log_message` function with the appropriate log level and message.
 * 
 * Otherwise you the macros below to log messages at different levels.
 * These macros will call the `log_wrap` function with the appropriate log level,
 * file, line, and message.
 
    void
    info(const char* format, ...) {
            va_list args;
            va_start(args, format);
            log_message("INFO", GREEN_COLOR, format, args);
            va_end(args);
    }
    
    void
    debug(const char* format, ...) {
            va_list args;
            va_start(args, format);
            log_message("DEBUG", BLUE_COLOR, format, args);
            va_end(args);
    }
    
    void
    warn(const char* format, ...) {
            va_list args;
            va_start(args, format);
            log_message("WARN", YELLOW_COLOR, format, args);
            va_end(args);
    }
    
    void
    error(const char* format, ...) {
            va_list args;
            va_start(args, format);
            log_message("ERROR", RED_COLOR, format, args);
            va_end(args);
    }
*/

#ifndef LOG_H
#define LOG_H

/**
 * @brief Log an informational message
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 * void info(const char* format, ...);
 */

/**
 * @brief Log a debug message
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 * void debug(const char* format, ...);
 */

/**
 * @brief Log a warning message
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 * void warn(const char* format, ...);
 */

/**
 * @brief Log an error message
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 * void error(const char* format, ...);
 */

#define info(fmt, ...)    log_wrap(LogLevel_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define debug(fmt, ...)   log_wrap(LogLevel_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define warning(fmt, ...) log_wrap(LogLevel_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define error(fmt, ...)   log_wrap(LogLevel_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...)   log_wrap(LogLevel_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define dblog(fmt, ...)   logdb(LogLevel_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define replog(fmt, ...)  logrepl(LogLevel_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

typedef enum {
        LogLevel_INFO,
        LogLevel_DEBUG,
        LogLevel_DEV,
        LogLevel_WARN,
        LogLevel_ERROR,
        LogLevel_FATAL,
} LogLevel;

/*
* Var. log wrapper to accept/reject log requests given the global log level
* called by the macros defined above. DO NOT CALL THIS DIRECTLY
*/
void log_wrap(LogLevel level, const char* file, int line, const char* message, ...);
void logdb(LogLevel level, const char* file, int line, const char* message, ...);
void logrepl(LogLevel level, const char* file, int line, const char* message, ...);
#endif // LOG_H
