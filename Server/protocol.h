#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Versión del protocolo
#define PROTOCOL_VERSION "VATP/1.0"

// Tamaños de buffer
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 50
#define MAX_USERNAME 32
#define MAX_PASSWORD 64
#define MAX_TOKEN 128

// Tipos de mensaje
typedef enum {
    MSG_CONNECT,
    MSG_AUTH,
    MSG_GET_TELEMETRY,
    MSG_COMMAND,
    MSG_LIST_USERS,
    MSG_DISCONNECT,
    MSG_RESPONSE_OK,
    MSG_RESPONSE_ERROR,
    MSG_TELEMETRY_DATA
} MessageType;

// Tipos de usuario
typedef enum {
    USER_OBSERVER,
    USER_ADMIN
} UserType;

// Comandos de control
typedef enum {
    CMD_SPEED_UP,
    CMD_SLOW_DOWN,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT,
    CMD_UNKNOWN
} CommandType;

// Estado del vehículo
typedef struct {
    float speed;           // km/h
    float battery;         // porcentaje
    float temperature;     // grados celsius
    char direction[16];    // "NORTH", "SOUTH", "EAST", "WEST"
    int is_moving;
} VehicleState;

// Información del cliente
typedef struct {
    int socket_fd;
    char ip[16];
    int port;
    UserType user_type;
    char username[MAX_USERNAME];
    char auth_token[MAX_TOKEN];
    int authenticated;
    int active;
} ClientInfo;

// Estructura de mensaje
typedef struct {
    char version[16];
    MessageType type;
    int length;
    char username[MAX_USERNAME];
    char auth_token[MAX_TOKEN];
    char command[32];
    char data[BUFFER_SIZE];
} Message;

// Funciones del protocolo
int parse_message(const char* raw_msg, Message* msg);
int build_response(char* buffer, MessageType type, const char* data);
int build_telemetry_message(char* buffer, VehicleState* state);
CommandType parse_command(const char* cmd_str);
const char* command_to_string(CommandType cmd);
const char* message_type_to_string(MessageType type);

#endif // PROTOCOL_H