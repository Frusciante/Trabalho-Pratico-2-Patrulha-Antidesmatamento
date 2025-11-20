#include "util.h"
#include "threads.h"

int is_running;

static void sig_handler(int sig)
{
    is_running = 0;
}

int main(void)
{
    pthread_t threads[THREAD_CNT] = {};
    int i;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sig_handler);
    
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

    return 0;
}