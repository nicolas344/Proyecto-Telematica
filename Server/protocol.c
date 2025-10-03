#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int parse_message(const char* raw_msg, Message* msg) {
    // Copiar el mensaje raw para parsearlo
    char buffer[BUFFER_SIZE];
    strncpy(buffer, raw_msg, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    // Limpiar la estructura
    memset(msg, 0, sizeof(Message));
    
    // Parsear primera línea: "VATP/1.0 TYPE LENGTH"
    char* line = strtok(buffer, "\r\n");
    if (!line) return 0;
    
    char version[16], type_str[32];
    int length;
    
    if (sscanf(line, "%s %s %d", version, type_str, &length) != 3) {
        return 0;
    }
    
    strncpy(msg->version, version, 15);
    msg->length = length;
    
    // Determinar tipo de mensaje
    if (strcmp(type_str, "CONNECT") == 0) {
        msg->type = MSG_CONNECT;
    } else if (strcmp(type_str, "AUTH") == 0) {
        msg->type = MSG_AUTH;
    } else if (strcmp(type_str, "GET_TELEMETRY") == 0) {
        msg->type = MSG_GET_TELEMETRY;
    } else if (strcmp(type_str, "COMMAND") == 0) {
        msg->type = MSG_COMMAND;
    } else if (strcmp(type_str, "LIST_USERS") == 0) {
        msg->type = MSG_LIST_USERS;
    } else if (strcmp(type_str, "DISCONNECT") == 0) {
        msg->type = MSG_DISCONNECT;
    } else {
        return 0; // Tipo desconocido
    }
    
    // Parsear headers
    while ((line = strtok(NULL, "\r\n")) != NULL) {
        if (strlen(line) == 0) break; // Línea vacía = fin de headers
        
        char key[64], value[BUFFER_SIZE];
        if (sscanf(line, "%[^:]: %[^\r\n]", key, value) == 2) {
            if (strcmp(key, "User-Type") == 0) {
                strncpy(msg->data, value, BUFFER_SIZE - 1);
            } else if (strcmp(key, "Username") == 0) {
                strncpy(msg->username, value, MAX_USERNAME - 1);
            } else if (strcmp(key, "Password") == 0) {
                strncpy(msg->auth_token, value, MAX_TOKEN - 1);
            } else if (strcmp(key, "Command") == 0) {
                strncpy(msg->command, value, 31);
            }
        }
    }
    
    // Si hay body después de los headers (para algunos mensajes)
    if (line != NULL && strlen(line) > 0) {
        line = strtok(NULL, "\0"); // Resto del mensaje
        if (line) {
            strncpy(msg->data, line, BUFFER_SIZE - 1);
        }
    }
    
    return 1; // Éxito
}

int build_response(char* buffer, MessageType type, const char* data) {
    const char* type_str = message_type_to_string(type);
    int length = data ? strlen(data) : 0;
    
    sprintf(buffer, "%s %s %d\r\n\r\n%s", 
            PROTOCOL_VERSION, type_str, length, data ? data : "");
    
    return strlen(buffer);
}

int build_telemetry_message(char* buffer, VehicleState* state) {
    char data[512];
    sprintf(data, "Speed: %.2f km/h\r\nBattery: %.2f%%\r\nTemperature: %.2f C\r\n"
                  "Direction: %s\r\nMoving: %s",
            state->speed, state->battery, state->temperature,
            state->direction, state->is_moving ? "Yes" : "No");
    
    return build_response(buffer, MSG_TELEMETRY_DATA, data);
}

CommandType parse_command(const char* cmd_str) {
    if (strcmp(cmd_str, "SPEED_UP") == 0) return CMD_SPEED_UP;
    if (strcmp(cmd_str, "SLOW_DOWN") == 0) return CMD_SLOW_DOWN;
    if (strcmp(cmd_str, "TURN_LEFT") == 0) return CMD_TURN_LEFT;
    if (strcmp(cmd_str, "TURN_RIGHT") == 0) return CMD_TURN_RIGHT;
    return CMD_UNKNOWN;
}

const char* command_to_string(CommandType cmd) {
    switch (cmd) {
        case CMD_SPEED_UP: return "SPEED_UP";
        case CMD_SLOW_DOWN: return "SLOW_DOWN";
        case CMD_TURN_LEFT: return "TURN_LEFT";
        case CMD_TURN_RIGHT: return "TURN_RIGHT";
        default: return "UNKNOWN";
    }
}

const char* message_type_to_string(MessageType type) {
    switch (type) {
        case MSG_CONNECT: return "CONNECT";
        case MSG_AUTH: return "AUTH";
        case MSG_GET_TELEMETRY: return "GET_TELEMETRY";
        case MSG_COMMAND: return "COMMAND";
        case MSG_LIST_USERS: return "LIST_USERS";
        case MSG_DISCONNECT: return "DISCONNECT";
        case MSG_RESPONSE_OK: return "RESPONSE_OK";
        case MSG_RESPONSE_ERROR: return "RESPONSE_ERROR";
        case MSG_TELEMETRY_DATA: return "TELEMETRY_DATA";
        default: return "UNKNOWN";
    }
}