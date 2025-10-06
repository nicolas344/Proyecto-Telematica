// ============= logger.c =============
#include "logger.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>

static FILE* log_file_handle = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Función unificada de logging
static void write_log(FILE* stream, const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(stream, "[%s] ", timestamp);
    if (log_file_handle && stream != log_file_handle) {
        fprintf(log_file_handle, "[%s] ", timestamp);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    if (log_file_handle && stream != log_file_handle) {
        va_start(args, format);
        vfprintf(log_file_handle, format, args);
        fflush(log_file_handle);
    }
    va_end(args);
    
    fflush(stream);
    pthread_mutex_unlock(&log_mutex);
}

void logger_init(const char* log_file) {
    pthread_mutex_lock(&log_mutex);
    
    log_file_handle = fopen(log_file, "a");
    if (log_file_handle == NULL) {
        fprintf(stderr, "ERROR: No se pudo abrir el archivo de log: %s\n", log_file);
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    // Log de inicio
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file_handle, "\n========== SERVER STARTED [%s] ==========\n", timestamp);
    fprintf(stdout, "\n========== SERVER STARTED [%s] ==========\n", timestamp);
    fflush(log_file_handle);
    
    pthread_mutex_unlock(&log_mutex);
}

void logger_close() {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file_handle != NULL) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(log_file_handle, "========== SERVER STOPPED [%s] ==========\n\n", timestamp);
        fclose(log_file_handle);
        log_file_handle = NULL;
    }
    
    pthread_mutex_unlock(&log_mutex);
}

void log_message(const char* client_ip, int client_port, const char* msg_type, const char* content) {
    write_log(stdout, "CLIENT[%s:%d] %s: %s\n", client_ip, client_port, msg_type, content);
}

void log_error(const char* error_msg) {
    write_log(stderr, "ERROR: %s\n", error_msg);
}

void log_info(const char* info_msg) {
    write_log(stdout, "INFO: %s\n", info_msg);
}