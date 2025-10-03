// ============= auth.h =============
#ifndef AUTH_H
#define AUTH_H

#include "protocol.h"

void auth_init();
int authenticate_user(const char* username, const char* password, char* token_out);
int validate_token(const char* username, const char* token);
void revoke_token(const char* username);

#endif // AUTH_H