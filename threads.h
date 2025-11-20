#include <pthread.h>

void* thread_monitoring_simulator(void* arg);
void* thread_telemetry_sender(void* arg);
void* thread_drone_team_msg_receiver(void* arg);
void* thread_drone_team_action_simulator(void* arg);