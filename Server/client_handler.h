
// ============= client_handler.h =============
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "protocol.h"

void* handle_client(void* arg);
int add_client(int socket_fd, const char* ip, int port);
void remove_client(int socket_fd);
void list_connected_users(char* buffer);

#endif // CLIENT_HANDLER_H