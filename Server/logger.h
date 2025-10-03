// ============= logger.h =============
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <pthread.h>

void logger_init(const char* log_file);
void logger_close();
void log_message(const char* client_ip, int client_port, const char* msg_type, const char* content);
void log_error(const char* error_msg);
void log_info(const char* info_msg);

#endif // LOGGER_H