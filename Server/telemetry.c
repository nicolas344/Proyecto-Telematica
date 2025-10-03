#include "telemetry.h"
#include "logger.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

VehicleState vehicle_state;
pthread_mutex_t vehicle_mutex = PTHREAD_MUTEX_INITIALIZER;

// Variables externas del servidor
extern ClientInfo clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

void telemetry_init() {
    pthread_mutex_lock(&vehicle_mutex);
    
    vehicle_state.speed = 0.0;
    vehicle_state.battery = 100.0;
    vehicle_state.temperature = 25.0;
    strcpy(vehicle_state.direction, "NORTH");
    vehicle_state.is_moving = 0;
    
    pthread_mutex_unlock(&vehicle_mutex);
    
    log_info("Sistema de telemetría inicializado");
}

// Simula cambios en el vehículo
void simulate_vehicle_changes() {
    pthread_mutex_lock(&vehicle_mutex);
    
    // Consumir batería si está en movimiento
    if (vehicle_state.is_moving && vehicle_state.battery > 0) {
        vehicle_state.battery -= 0.5;
        if (vehicle_state.battery < 0) vehicle_state.battery = 0;
    }
    
    // Temperatura varía ligeramente
    vehicle_state.temperature += ((float)(rand() % 20 - 10)) / 10.0;
    if (vehicle_state.temperature < 15.0) vehicle_state.temperature = 15.0;
    if (vehicle_state.temperature > 45.0) vehicle_state.temperature = 45.0;
    
    // Detener si no hay batería
    if (vehicle_state.battery <= 5.0) {
        vehicle_state.speed = 0.0;
        vehicle_state.is_moving = 0;
    }
    
    pthread_mutex_unlock(&vehicle_mutex);
}

void* telemetry_broadcast_thread(void* arg) {
    char buffer[BUFFER_SIZE];
    
    log_info("Thread de telemetría iniciado (broadcast cada 10 segundos)");
    
    while (1) {
        sleep(10); // Esperar 10 segundos
        
        simulate_vehicle_changes();
        
        pthread_mutex_lock(&vehicle_mutex);
        int len = build_telemetry_message(buffer, &vehicle_state);
        pthread_mutex_unlock(&vehicle_mutex);
        
        // Enviar a todos los clientes activos
        pthread_mutex_lock(&clients_mutex);
        int sent_count = 0;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].socket_fd > 0) {
                int sent = send(clients[i].socket_fd, buffer, len, 0);
                if (sent > 0) {
                    sent_count++;
                } else {
                    // Cliente desconectado
                    clients[i].active = 0;
                    close(clients[i].socket_fd);
                    log_message(clients[i].ip, clients[i].port, "DISCONNECTED", 
                               "Cliente desconectado durante broadcast");
                }
            }
        }
        
        pthread_mutex_unlock(&clients_mutex);
        
        char log_msg[256];
        sprintf(log_msg, "Telemetría enviada a %d clientes", sent_count);
        log_info(log_msg);
    }
    
    return NULL;
}

int can_execute_command(CommandType command, char* reason) {
    pthread_mutex_lock(&vehicle_mutex);
    
    // Verificar batería baja
    if (vehicle_state.battery < 10.0) {
        strcpy(reason, "Batería demasiado baja");
        pthread_mutex_unlock(&vehicle_mutex);
        return 0;
    }
    
    // Verificar límite de velocidad
    if (command == CMD_SPEED_UP && vehicle_state.speed >= 100.0) {
        strcpy(reason, "Límite de velocidad alcanzado (100 km/h)");
        pthread_mutex_unlock(&vehicle_mutex);
        return 0;
    }
    
    if (command == CMD_SLOW_DOWN && vehicle_state.speed <= 0.0) {
        strcpy(reason, "Vehículo ya está detenido");
        pthread_mutex_unlock(&vehicle_mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&vehicle_mutex);
    return 1; // Comando puede ejecutarse
}

void update_vehicle_state(CommandType command) {
    pthread_mutex_lock(&vehicle_mutex);
    
    switch (command) {
        case CMD_SPEED_UP:
            vehicle_state.speed += 10.0;
            if (vehicle_state.speed > 100.0) vehicle_state.speed = 100.0;
            vehicle_state.is_moving = 1;
            break;
            
        case CMD_SLOW_DOWN:
            vehicle_state.speed -= 10.0;
            if (vehicle_state.speed < 0.0) vehicle_state.speed = 0.0;
            if (vehicle_state.speed == 0.0) vehicle_state.is_moving = 0;
            break;
            
        case CMD_TURN_LEFT:
            if (strcmp(vehicle_state.direction, "NORTH") == 0)
                strcpy(vehicle_state.direction, "WEST");
            else if (strcmp(vehicle_state.direction, "WEST") == 0)
                strcpy(vehicle_state.direction, "SOUTH");
            else if (strcmp(vehicle_state.direction, "SOUTH") == 0)
                strcpy(vehicle_state.direction, "EAST");
            else if (strcmp(vehicle_state.direction, "EAST") == 0)
                strcpy(vehicle_state.direction, "NORTH");
            break;
            
        case CMD_TURN_RIGHT:
            if (strcmp(vehicle_state.direction, "NORTH") == 0)
                strcpy(vehicle_state.direction, "EAST");
            else if (strcmp(vehicle_state.direction, "EAST") == 0)
                strcpy(vehicle_state.direction, "SOUTH");
            else if (strcmp(vehicle_state.direction, "SOUTH") == 0)
                strcpy(vehicle_state.direction, "WEST");
            else if (strcmp(vehicle_state.direction, "WEST") == 0)
                strcpy(vehicle_state.direction, "NORTH");
            break;
            
        default:
            break;
    }
    
    pthread_mutex_unlock(&vehicle_mutex);
}