#include "threads.h"

int is_running;
info_cidade_t* city_info;
int city_cnt;
pthread_mutex_t city_info_mutex;

static void sig_handler(int sig)
{
    is_running = 0;
}

int main(void)
{
    pthread_t threads[THREAD_CNT] = {};
    int i;
    
    pthread_mutex_init(&city_info_mutex, NULL);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sig_handler);
    
    if (get_info_from_file(FILENAME, &city_info, &city_cnt, NULL, NULL, NULL))
    {
        return 1;
    }

    is_running = 1;

    if (
        pthread_create(&threads[0], NULL, thread_monitoring_simulator, NULL) ||
        pthread_create(&threads[1], NULL, thread_telemetry_sender, NULL) ||
        pthread_create(&threads[2], NULL, thread_drone_team_msg_receiver, NULL) ||
        pthread_create(&threads[3], NULL, thread_drone_team_action_simulator, NULL)
    )
    {
        return 1;
    }
    
    for (i = 0; i < THREAD_CNT; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&city_info_mutex);

    return 0;
}