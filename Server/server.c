#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "protocol.h"
#include "logger.h"
#include "auth.h"
#include "telemetry.h"
#include "client_handler.h"

// Variables globales
ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_socket = -1;
volatile sig_atomic_t server_running = 1;

// Manejador de señales para limpieza
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n\nRecibida señal de terminación. Cerrando servidor...\n");
        server_running = 0;
        
        if (server_socket >= 0) {
            close(server_socket);
        }
    }
}

void init_clients() {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].socket_fd = -1;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void cleanup_all_clients() {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket_fd >= 0) {
            close(clients[i].socket_fd);
            clients[i].active = 0;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <puerto> <archivo_log>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 8080 server.log\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    char* log_file = argv[2];
    
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Puerto inválido. Debe estar entre 1 y 65535\n");
        return 1;
    }
    
    // Configurar manejadores de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Ignorar SIGPIPE
    
    // Inicializar sistemas
    printf("==============================================\n");
    printf("  SERVIDOR DE VEHÍCULO AUTÓNOMO DE TELEMETRÍA\n");
    printf("==============================================\n\n");
    
    logger_init(log_file);
    auth_init();
    telemetry_init();
    init_clients();
    
    char init_msg[256];
    sprintf(init_msg, "Servidor inicializado en puerto %d", port);
    log_info(init_msg);
    
    // Crear socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_error("No se pudo crear el socket");
        return 1;
    }
    
    // Configurar opciones del socket
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_error("Error configurando SO_REUSEADDR");
        close(server_socket);
        return 1;
    }
    
    // Configurar dirección del servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("Error en bind(). El puerto puede estar en uso");
        close(server_socket);
        return 1;
    }
    
    // Listen
    if (listen(server_socket, 10) < 0) {
        log_error("Error en listen()");
        close(server_socket);
        return 1;
    }
    
    char listen_msg[256];
    sprintf(listen_msg, "Servidor escuchando en puerto %d", port);
    log_info(listen_msg);
    printf("\n✓ Servidor listo para recibir conexiones en puerto %d\n", port);
    printf("✓ Logs guardándose en: %s\n\n", log_file);
    printf("Presione Ctrl+C para detener el servidor\n");
    printf("==============================================\n\n");
    
    // Iniciar thread de telemetría
    pthread_t telemetry_thread;
    if (pthread_create(&telemetry_thread, NULL, telemetry_broadcast_thread, NULL) != 0) {
        log_error("No se pudo crear thread de telemetría");
        close(server_socket);
        return 1;
    }
    pthread_detach(telemetry_thread);
    
    // Loop principal - aceptar clientes
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Aceptar conexión
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (server_running) {
                log_error("Error aceptando conexión");
            }
            continue;
        }
        
        if (!server_running) {
            close(client_socket);
            break;
        }
        
        // Información del cliente
        char client_ip[16];
        strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
        int client_port = ntohs(client_addr.sin_port);
        
        char accept_msg[256];
        sprintf(accept_msg, "Nueva conexión aceptada desde %s:%d", client_ip, client_port);
        log_info(accept_msg);
        
        // Crear thread para manejar el cliente
        pthread_t client_thread;
        int* socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;
        
        if (pthread_create(&client_thread, NULL, handle_client, socket_ptr) != 0) {
            log_error("No se pudo crear thread para el cliente");
            close(client_socket);
            free(socket_ptr);
            continue;
        }
        
        // Detach thread para que se limpie automáticamente
        pthread_detach(client_thread);
    }
    
    // Limpieza
    printf("\nCerrando servidor...\n");
    log_info("Iniciando proceso de cierre del servidor");
    
    cleanup_all_clients();
    
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    log_info("Servidor cerrado correctamente");
    logger_close();
    
    printf("Servidor terminado.\n");
    
    return 0;
}