#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h> // For flock
#include <unistd.h>   // For fileno
#include "logging.h"

// Mutex for thread safety within the same process (e.g., server threads)
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

// Store the process name (SERVER or CLIENT)
static char current_process_name[32] = "UNKNOWN";

void log_init(const char *process_name) {
    strncpy(current_process_name, process_name, sizeof(current_process_name) - 1);
    current_process_name[sizeof(current_process_name) - 1] = '\0';
}

void log_message(LogLevel level, const char *format, ...) {
    // 1. Prepare Timestamp
    time_t now;
    time(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // 2. Resolve Level String
    const char *level_str = "INFO";
    if (level == LOG_WARN) level_str = "WARN";
    else if (level == LOG_ERROR) level_str = "ERROR";

    // 3. Format the actual message content into a buffer
    // We do this once so we can write the exact same string to stdout and file
    char message_buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);

    // --- Output 1: Console (Thread Safe) ---
    pthread_mutex_lock(&log_lock);
    printf("[%s] [%s] [TID: %lu] [%s] %s\n", time_str, current_process_name, (unsigned long)pthread_self(), level_str, message_buffer);
    pthread_mutex_unlock(&log_lock);

    // --- Output 2: File (Process Safe via flock) ---
    FILE *fp = fopen("application.log", "a");
    if (fp) {
        int fd = fileno(fp);
        
        // Acquire Exclusive Lock (Blocks if another process has it)
        if (flock(fd, LOCK_EX) == 0) {
            fprintf(fp, "[%s] [%s] [TID: %lu] [%s] %s\n", time_str, current_process_name, (unsigned long)pthread_self(), level_str, message_buffer);
            fflush(fp); // Ensure it's written physically
            flock(fd, LOCK_UN); // Release Lock
        } else {
            // If lock failed (rare), strictly fallback to stderr so we know
            fprintf(stderr, "Logging lock failed!\n");
        }
        fclose(fp);
    }
}
