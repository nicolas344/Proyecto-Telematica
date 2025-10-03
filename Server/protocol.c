#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char* message_type_to_string(MessageType type) {
    switch(type) {
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

const char* command_to_string(CommandType cmd) {
    switch(cmd) {
        case CMD_SPEED_UP: return "SPEED_UP";
        case CMD_SLOW_DOWN: return "SLOW_DOWN";
        case CMD_TURN_LEFT: return "TURN_LEFT";
        case CMD_TURN_RIGHT: return "TURN_RIGHT";
        default: return "UNKNOWN";
    }
}

CommandType parse_command(const char* cmd_str) {
    if (strcmp(cmd_str, "SPEED_UP") == 0 || strcmp(cmd_str, "SPEED UP") == 0)
        return CMD_SPEED_UP;
    if (strcmp(cmd_str, "SLOW_DOWN") == 0 || strcmp(cmd_str, "SLOW DOWN") == 0)
        return CMD_SLOW_DOWN;
    if (strcmp(cmd_str, "TURN_LEFT") == 0 || strcmp(cmd_str, "TURN LEFT") == 0)
        return CMD_TURN_LEFT;
    if (strcmp(cmd_str, "TURN_RIGHT") == 0 || strcmp(cmd_str, "TURN RIGHT") == 0)
        return CMD_TURN_RIGHT;
    return CMD_UNKNOWN;
}

// Parser de mensajes entrantes
int parse_message(const char* raw_msg, Message* msg) {
    if (raw_msg == NULL || msg == NULL) return 0;
    
    memset(msg, 0, sizeof(Message));
    
    // Copiar para parsear
    char buffer[BUFFER_SIZE];
    strncpy(buffer, raw_msg, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    // Parsear línea por línea
    char* line = strtok(buffer, "\r\n");
    if (line == NULL) return 0;
    
    // Primera línea: VERSION TYPE LENGTH
    char type_str[32];
    if (sscanf(line, "%s %s %d", msg->version, type_str, &msg->length) != 3) {
        return 0;
    }
    
    // Determinar tipo de mensaje
    if (strcmp(type_str, "CONNECT") == 0) msg->type = MSG_CONNECT;
    else if (strcmp(type_str, "AUTH") == 0) msg->type = MSG_AUTH;
    else if (strcmp(type_str, "GET_TELEMETRY") == 0) msg->type = MSG_GET_TELEMETRY;
    else if (strcmp(type_str, "COMMAND") == 0) msg->type = MSG_COMMAND;
    else if (strcmp(type_str, "LIST_USERS") == 0) msg->type = MSG_LIST_USERS;
    else if (strcmp(type_str, "DISCONNECT") == 0) msg->type = MSG_DISCONNECT;
    else return 0;
    
    // Parsear headers
    while ((line = strtok(NULL, "\r\n")) != NULL) {
        if (strlen(line) == 0) break; // Línea vacía indica fin de headers
        
        char key[64], value[BUFFER_SIZE];
        if (sscanf(line, "%[^:]: %[^\n]", key, value) == 2) {
            if (strcmp(key, "Username") == 0) {
                strncpy(msg->username, value, MAX_USERNAME - 1);
            } else if (strcmp(key, "Password") == 0 || strcmp(key, "Auth-Token") == 0) {
                strncpy(msg->auth_token, value, MAX_TOKEN - 1);
            } else if (strcmp(key, "Command") == 0) {
                strncpy(msg->command, value, 31);
            } else if (strcmp(key, "User-Type") == 0) {
                strncpy(msg->data, value, BUFFER_SIZE - 1);
            }
        }
    }
    
    return 1;
}

int build_response(char* buffer, MessageType type, const char* data) {
    int offset = 0;
    
    const char* type_str = message_type_to_string(type);
    int data_len = data ? strlen(data) : 0;
    
    offset += sprintf(buffer + offset, "%s %s %d\r\n", PROTOCOL_VERSION, type_str, data_len);
    
    if (data && data_len > 0) {
        offset += sprintf(buffer + offset, "\r\n%s", data);
    } else {
        offset += sprintf(buffer + offset, "\r\n");
    }
    
    return offset;
}

int build_telemetry_message(char* buffer, VehicleState* state) {
    char data[512];
    sprintf(data, "Speed: %.2f km/h\r\nBattery: %.2f%%\r\nTemperature: %.2f C\r\nDirection: %s\r\nMoving: %s",
            state->speed, state->battery, state->temperature, state->direction,
            state->is_moving ? "Yes" : "No");
    
    return build_response(buffer, MSG_TELEMETRY_DATA, data);
}