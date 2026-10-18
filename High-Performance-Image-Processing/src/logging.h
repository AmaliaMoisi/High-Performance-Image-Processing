#ifndef LOGGING_H
#define LOGGING_H

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

// Initialize the logger with a process name (e.g., "SERVER", "CLIENT")
void log_init(const char *process_name);

// Log a message
void log_message(LogLevel level, const char *format, ...);

#endif
