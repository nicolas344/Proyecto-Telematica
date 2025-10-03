// ============= auth.c =============
#include "auth.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// Base de datos simple de usuarios (en producción usar BD)
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char token[MAX_TOKEN];
    time_t token_expiry;
} UserCredential;

#define MAX_USERS 10
static UserCredential users[MAX_USERS];
static int user_count = 0;

void auth_init() {
    // Inicializar con algunos usuarios de prueba
    strcpy(users[0].username, "admin");
    strcpy(users[0].password, "admin123");
    users[0].token[0] = '\0';
    users[0].token_expiry = 0;
    user_count = 1;
    
    strcpy(users[1].username, "admin2");
    strcpy(users[1].password, "pass456");
    users[1].token[0] = '\0';
    users[1].token_expiry = 0;
    user_count = 2;
}

// Genera un token simple (en producción usar UUID o JWT)
static void generate_token(char* token_out) {
    srand(time(NULL) + rand());
    sprintf(token_out, "TOKEN_%ld_%d", time(NULL), rand() % 100000);
}

int authenticate_user(const char* username, const char* password, char* token_out) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            
            // Generar nuevo token
            generate_token(users[i].token);
            users[i].token_expiry = time(NULL) + 3600; // Token válido por 1 hora
            
            strcpy(token_out, users[i].token);
            return 1; // Autenticación exitosa
        }
    }
    return 0; // Credenciales inválidas
}

int validate_token(const char* username, const char* token) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            // Verificar token y expiración
            if (strcmp(users[i].token, token) == 0 &&
                time(NULL) < users[i].token_expiry) {
                return 1; // Token válido
            }
            return 0; // Token inválido o expirado
        }
    }
    return 0; // Usuario no encontrado
}

void revoke_token(const char* username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            users[i].token[0] = '\0';
            users[i].token_expiry = 0;
            return;
        }
    }
}
