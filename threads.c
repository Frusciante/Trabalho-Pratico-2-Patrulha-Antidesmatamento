#include "threads.h"

extern int is_running;
extern info_cidade_t* city_info;
extern pthread_mutex_t city_info_mutex;
extern int city_cnt;

void* thread_monitoring_simulator(void* arg)
{
    int i;
    srand(time(NULL));

    while (is_running)
    {
        pthread_mutex_lock(&city_info_mutex); 
        for (i = 0; i < city_cnt; ++i) if (rand() % 100 < 3)    
        {
            city_info[i].status = ALERTA;
        }
        pthread_mutex_unlock(&city_info_mutex);
        sleep(SLEEP_TIME);
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