// ============= client_handler.c =============
#include "client_handler.h"
#include "logger.h"
#include "auth.h"
#include "telemetry.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

extern ClientInfo clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

int add_client(int socket_fd, const char* ip, int port) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket_fd = socket_fd;
            strncpy(clients[i].ip, ip, 15);
            clients[i].ip[15] = '\0';
            clients[i].port = port;
            clients[i].user_type = USER_OBSERVER;
            clients[i].authenticated = 0;
            clients[i].active = 1;
            clients[i].username[0] = '\0';
            clients[i].auth_token[0] = '\0';
            
            pthread_mutex_unlock(&clients_mutex);
            
            char log_msg[256];
            sprintf(log_msg, "Cliente añadido al sistema");
            log_message(ip, port, "CONNECTED", log_msg);
            
            return i;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    return -1; // No hay espacio
}

void remove_client(int socket_fd) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket_fd == socket_fd) {
            log_message(clients[i].ip, clients[i].port, "REMOVED", "Cliente removido del sistema");
            
            clients[i].active = 0;
            clients[i].socket_fd = -1;
            close(socket_fd);
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void list_connected_users(char* buffer) {
    pthread_mutex_lock(&clients_mutex);
    
    int offset = 0;
    offset += sprintf(buffer + offset, "=== USUARIOS CONECTADOS ===\r\n");
    
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            count++;
            offset += sprintf(buffer + offset, "%d. [%s:%d] - %s - %s\r\n",
                            count,
                            clients[i].ip,
                            clients[i].port,
                            clients[i].user_type == USER_ADMIN ? "ADMIN" : "OBSERVER",
                            clients[i].authenticated ? clients[i].username : "No autenticado");
        }
    }
    
    if (count == 0) {
        offset += sprintf(buffer + offset, "No hay usuarios conectados\r\n");
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Función auxiliar para verificar admin autenticado
static int check_admin_auth(int client_idx, int client_socket, const char* client_ip, int client_port) {
    if (clients[client_idx].user_type != USER_ADMIN || !clients[client_idx].authenticated) {
        log_message(client_ip, client_port, "AUTH_ERROR", "No autorizado");
        char response[BUFFER_SIZE];
        build_response(response, MSG_RESPONSE_ERROR, "Debe ser administrador autenticado");
        send(client_socket, response, strlen(response), 0);
        return 0;
    }
    
    if (!validate_token(clients[client_idx].username, clients[client_idx].auth_token)) {
        log_message(client_ip, client_port, "TOKEN_ERROR", "Token inválido o expirado");
        char response[BUFFER_SIZE];
        build_response(response, MSG_RESPONSE_ERROR, "Token inválido. Reautentíquese");
        send(client_socket, response, strlen(response), 0);
        return 0;
    }
    
    return 1;
}

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    char buffer[BUFFER_SIZE];
    char accumulated[BUFFER_SIZE * 2] = {0}; // Buffer acumulado
    int acc_len = 0;
    char response[BUFFER_SIZE];
    Message msg;
    
    // Obtener información del cliente
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(client_socket, (struct sockaddr*)&addr, &addr_len);
    
    char client_ip[16];
    strcpy(client_ip, inet_ntoa(addr.sin_addr));
    int client_port = ntohs(addr.sin_port);
    
    // Agregar cliente a la lista
    int client_idx = add_client(client_socket, client_ip, client_port);
    if (client_idx < 0) {
        log_error("Máximo número de clientes alcanzado");
        close(client_socket);
        return NULL;
    }
    
    // Loop principal del cliente
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            // Cliente desconectado
            log_message(client_ip, client_port, "DISCONNECTED", "Conexión cerrada");
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        // Acumular el mensaje recibido
        if (acc_len + bytes_received < BUFFER_SIZE * 2 - 1) {
            strcat(accumulated, buffer);
            acc_len += bytes_received;
        }
        
        // Buscar el fin del mensaje (\n\n o \r\n\r\n)
        char* msg_end = strstr(accumulated, "\n\n");
        if (!msg_end) {
            msg_end = strstr(accumulated, "\r\n\r\n");
        }
        
        if (!msg_end) {
            // Mensaje incompleto, seguir acumulando
            continue;
        }
        
        // Tenemos un mensaje completo
        *msg_end = '\0'; // Terminar el mensaje
        
        // Parsear mensaje
        if (!parse_message(accumulated, &msg)) {
            log_message(client_ip, client_port, "ERROR", "Mensaje mal formado");
            build_response(response, MSG_RESPONSE_ERROR, "Formato de mensaje inválido");
            send(client_socket, response, strlen(response), 0);
            
            // Limpiar buffer acumulado
            memset(accumulated, 0, sizeof(accumulated));
            acc_len = 0;
            continue;
        }
        
        // Limpiar buffer acumulado para el siguiente mensaje
        memset(accumulated, 0, sizeof(accumulated));
        acc_len = 0;
        
        // Procesar según tipo de mensaje
        switch (msg.type) {
            case MSG_CONNECT: {
                // Conectar cliente
                UserType user_type = strcmp(msg.data, "ADMIN") == 0 ? USER_ADMIN : USER_OBSERVER;
                
                pthread_mutex_lock(&clients_mutex);
                clients[client_idx].user_type = user_type;
                pthread_mutex_unlock(&clients_mutex);
                
                char log_msg[256];
                sprintf(log_msg, "Solicitud de conexión como %s", 
                       user_type == USER_ADMIN ? "ADMIN" : "OBSERVER");
                log_message(client_ip, client_port, "CONNECT", log_msg);
                
                if (user_type == USER_ADMIN) {
                    build_response(response, MSG_RESPONSE_OK, 
                                 "Conectado como ADMIN. Debe autenticarse para enviar comandos");
                } else {
                    build_response(response, MSG_RESPONSE_OK, 
                                 "Conectado como OBSERVER. Recibirá telemetría automáticamente");
                }
                send(client_socket, response, strlen(response), 0);
                break;
            }
            
            case MSG_AUTH: {
                // Autenticar administrador
                if (clients[client_idx].user_type != USER_ADMIN) {
                    log_message(client_ip, client_port, "AUTH_ERROR", 
                               "Usuario no es administrador");
                    build_response(response, MSG_RESPONSE_ERROR, 
                                 "Solo administradores pueden autenticarse");
                    send(client_socket, response, strlen(response), 0);
                    break;
                }
                
                // Parsear usuario y contraseña
                char username[MAX_USERNAME];
                char password[MAX_PASSWORD];
                char token[MAX_TOKEN];
                
                // El username viene en msg.username y password en msg.auth_token
                strncpy(username, msg.username, MAX_USERNAME - 1);
                strncpy(password, msg.auth_token, MAX_PASSWORD - 1);
                
                if (authenticate_user(username, password, token)) {
                    pthread_mutex_lock(&clients_mutex);
                    clients[client_idx].authenticated = 1;
                    strncpy(clients[client_idx].username, username, MAX_USERNAME - 1);
                    strncpy(clients[client_idx].auth_token, token, MAX_TOKEN - 1);
                    pthread_mutex_unlock(&clients_mutex);
                    
                    char log_msg[256];
                    sprintf(log_msg, "Autenticación exitosa para usuario: %s", username);
                    log_message(client_ip, client_port, "AUTH_SUCCESS", log_msg);
                    
                    char resp_data[256];
                    sprintf(resp_data, "Autenticación exitosa. Token: %s", token);
                    build_response(response, MSG_RESPONSE_OK, resp_data);
                } else {
                    log_message(client_ip, client_port, "AUTH_FAILED", 
                               "Credenciales inválidas");
                    build_response(response, MSG_RESPONSE_ERROR, 
                                 "Credenciales inválidas");
                }
                send(client_socket, response, strlen(response), 0);
                break;
            }
            
            case MSG_COMMAND: {
                if (!check_admin_auth(client_idx, client_socket, client_ip, client_port)) break;
                
                CommandType cmd = parse_command(msg.command);
                if (cmd == CMD_UNKNOWN) {
                    log_message(client_ip, client_port, "COMMAND_ERROR", "Comando desconocido");
                    build_response(response, MSG_RESPONSE_ERROR, "Comando no reconocido");
                    send(client_socket, response, strlen(response), 0);
                    break;
                }
                
                char reason[256];
                if (!can_execute_command(cmd, reason)) {
                    log_message(client_ip, client_port, "COMMAND_REJECTED", reason);
                    build_response(response, MSG_RESPONSE_ERROR, reason);
                    send(client_socket, response, strlen(response), 0);
                    break;
                }
                
                update_vehicle_state(cmd);
                
                pthread_mutex_lock(&vehicle_mutex);
                sprintf(response, "Comando %s ejecutado. Speed: %.2f km/h, Direction: %s",
                       command_to_string(cmd), vehicle_state.speed, vehicle_state.direction);
                pthread_mutex_unlock(&vehicle_mutex);
                
                log_message(client_ip, client_port, "COMMAND_OK", response);
                build_response(response, MSG_RESPONSE_OK, response);
                send(client_socket, response, strlen(response), 0);
                break;
            }
            
            case MSG_LIST_USERS: {
                if (!check_admin_auth(client_idx, client_socket, client_ip, client_port)) break;
                
                char user_list[BUFFER_SIZE];
                list_connected_users(user_list);
                build_response(response, MSG_RESPONSE_OK, user_list);
                send(client_socket, response, strlen(response), 0);
                log_message(client_ip, client_port, "LIST_USERS", "OK");
                break;
            }
            
            case MSG_GET_TELEMETRY: {
                // Enviar telemetría inmediata
                pthread_mutex_lock(&vehicle_mutex);
                build_telemetry_message(response, &vehicle_state);
                pthread_mutex_unlock(&vehicle_mutex);
                
                log_message(client_ip, client_port, "GET_TELEMETRY", 
                           "Solicitó telemetría");
                
                send(client_socket, response, strlen(response), 0);
                break;
            }
            
            case MSG_DISCONNECT: {
                log_message(client_ip, client_port, "DISCONNECT", 
                           "Cliente solicitó desconexión");
                build_response(response, MSG_RESPONSE_OK, "Desconectado correctamente");
                send(client_socket, response, strlen(response), 0);
                goto cleanup;
            }
            
            default:
                log_message(client_ip, client_port, "ERROR", "Tipo de mensaje no soportado");
                build_response(response, MSG_RESPONSE_ERROR, "Tipo de mensaje no soportado");
                send(client_socket, response, strlen(response), 0);
                break;
        }
    }
    
cleanup:
    remove_client(client_socket);
    return NULL;
}