#include "threads.h"

extern int is_running;

void* thread_monitoring_simulator(void* arg)
{
    while (is_running)
    {
    }

    return NULL;
}

void* thread_telemetry_sender(void* arg)
{
    while (is_running)
    {
    }

    return NULL;
}

void* thread_drone_team_msg_receiver(void* arg)
{
    while (is_running)
    {
    }
    
    return NULL;
}

void* thread_drone_team_action_simulator(void* arg)
{
    while (is_running)
    {
    }

    return NULL;
}