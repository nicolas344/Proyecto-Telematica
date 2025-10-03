// ============= logger.c =============
#include "logger.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

static FILE* log_file_handle = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char log_entry[2048];
    snprintf(log_entry, sizeof(log_entry), 
             "[%s] CLIENT[%s:%d] %s: %s\n", 
             timestamp, client_ip, client_port, msg_type, content);
    
    // Escribir en archivo
    if (log_file_handle != NULL) {
        fprintf(log_file_handle, "%s", log_entry);
        fflush(log_file_handle);
    }
    
    // Escribir en consola
    fprintf(stdout, "%s", log_entry);
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_error(const char* error_msg) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char log_entry[2048];
    snprintf(log_entry, sizeof(log_entry), "[%s] ERROR: %s\n", timestamp, error_msg);
    
    if (log_file_handle != NULL) {
        fprintf(log_file_handle, "%s", log_entry);
        fflush(log_file_handle);
    }
    
    fprintf(stderr, "%s", log_entry);
    fflush(stderr);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_info(const char* info_msg) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char log_entry[2048];
    snprintf(log_entry, sizeof(log_entry), "[%s] INFO: %s\n", timestamp, info_msg);
    
    if (log_file_handle != NULL) {
        fprintf(log_file_handle, "%s", log_entry);
        fflush(log_file_handle);
    }
    
    fprintf(stdout, "%s", log_entry);
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}