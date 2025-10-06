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

// Tabla de comandos
static const struct {
    const char* name;
    CommandType type;
} command_table[] = {
    {"SPEED_UP", CMD_SPEED_UP},
    {"SLOW_DOWN", CMD_SLOW_DOWN},
    {"TURN_LEFT", CMD_TURN_LEFT},
    {"TURN_RIGHT", CMD_TURN_RIGHT},
    {NULL, CMD_UNKNOWN}
};

CommandType parse_command(const char* cmd_str) {
    for (int i = 0; command_table[i].name != NULL; i++) {
        if (strcmp(cmd_str, command_table[i].name) == 0) {
            return command_table[i].type;
        }
    }
    return CMD_UNKNOWN;
}

const char* command_to_string(CommandType cmd) {
    for (int i = 0; command_table[i].name != NULL; i++) {
        if (command_table[i].type == cmd) {
            return command_table[i].name;
        }
    }
    return "UNKNOWN";
}

// Similar para tipos de mensaje
static const struct {
    const char* name;
    MessageType type;
} message_table[] = {
    {"CONNECT", MSG_CONNECT},
    {"AUTH", MSG_AUTH},
    {"GET_TELEMETRY", MSG_GET_TELEMETRY},
    {"COMMAND", MSG_COMMAND},
    {"LIST_USERS", MSG_LIST_USERS},
    {"DISCONNECT", MSG_DISCONNECT},
    {"RESPONSE_OK", MSG_RESPONSE_OK},
    {"RESPONSE_ERROR", MSG_RESPONSE_ERROR},
    {"TELEMETRY_DATA", MSG_TELEMETRY_DATA},
    {NULL, MSG_CONNECT}
};

const char* message_type_to_string(MessageType type) {
    for (int i = 0; message_table[i].name != NULL; i++) {
        if (message_table[i].type == type) {
            return message_table[i].name;
        }
    }
    return "UNKNOWN";
}