// ============= telemetry.h =============
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "protocol.h"
#include <pthread.h>

extern VehicleState vehicle_state;
extern pthread_mutex_t vehicle_mutex;

void telemetry_init();
void* telemetry_broadcast_thread(void* arg);
void update_vehicle_state(CommandType command);
int can_execute_command(CommandType command, char* reason);

#endif // TELEMETRY_H