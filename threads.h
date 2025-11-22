#ifndef _THREADS_H
#define _THREADS_Ho
#define _GNU_SOURCE

#include <pthread.h>
#include "util.h"
#define THREAD_CNT 4

void* thread_monitoring_simulator(void* arg);
void* thread_telemetry_sender(void* arg);
void* thread_msg_receiver(void* arg);
void* thread_drone_team_action_simulator(void* arg);

#endif