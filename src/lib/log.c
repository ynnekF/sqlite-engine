#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "log.h"

// ANSI color codes for different log levels
#define RESET_COLOR  "\033[0m"
#define RED_COLOR    "\033[31m"
#define YELLOW_COLOR "\033[33m"
#define BLUE_COLOR   "\033[34m"
#define GREEN_COLOR  "\033[32m"
#define GRAY_COLOR   "\033[90m"

#define LIGHT_GRAY_COLOR "\033[40m"
#define LIGHT_BLUE_COLOR "\033[44m"

// Internal function to get current timestamp
static void
get_timestamp(char* buffer, size_t size) {
        time_t rawtime;
        struct tm* timeinfo;

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Internal function to log with a specific level
static void
log_message(const char* level, const char* color, const char* format, va_list args) {
        char timestamp[64];
        get_timestamp(timestamp, sizeof(timestamp));

#if defined(NO_COLOR)
        // Print timestamp and level without color
        printf("[%s] [%s] ", timestamp, level);
#else
        // Print timestamp and level with color
        printf("%s[%s] [%s]%s ", color, timestamp, level, RESET_COLOR);
#endif

        // Print the actual message
        vprintf(format, args);
        printf("\n");
        fflush(stdout);
}

void
log_wrap(LogLevel level, const char* file, int line, const char* message, ...) {
        va_list args;
        va_start(args, message);
        switch (level) {
                case LogLevel_INFO: log_message("INFO", GREEN_COLOR, message, args); break;
                case LogLevel_DEBUG: log_message("DEBUG", BLUE_COLOR, message, args); break;
                case LogLevel_DEV: log_message("DEV", BLUE_COLOR, message, args); break;
                case LogLevel_WARN: log_message("WARN", YELLOW_COLOR, message, args); break;
                case LogLevel_ERROR: log_message("ERROR", RED_COLOR, message, args); break;
                case LogLevel_FATAL: log_message("FATAL", RED_COLOR, message, args); break;
                default: break;
        }
}

void
logdb(LogLevel level, const char* file, int line, const char* message, ...) {
        va_list args;
        va_start(args, message);

#if defined(NO_COLOR)
        // Print timestamp and level without color
        printf("%s", message);
#else
        // Print timestamp and level with color
        printf("%s(%s %d) ", GRAY_COLOR, file, line);

#endif
        vprintf(message, args);
        printf("%s\n", RESET_COLOR);
        fflush(stdout);
}

void
logrepl(LogLevel level, const char* file, int line, const char* message, ...) {
        va_list args;
        va_start(args, message);

        char prefix[128];
        int prefix_len;

#if defined(NO_COLOR)
        prefix_len = snprintf(prefix, sizeof(prefix), "(%s %d)", file, line);
#else
        prefix_len = snprintf(prefix, sizeof(prefix), "%s(%s %d)", YELLOW_COLOR, file, line);
#endif

        // Calculate padding to align message
        int padding = 30 - prefix_len;
        if (padding < 1)
                padding = 1;

        printf("%s%*s", prefix, padding, ""); // Print prefix + padding

        // Print the formatted message
        vprintf(message, args);
        printf("%s\n", RESET_COLOR);

        va_end(args);
        fflush(stdout);
}