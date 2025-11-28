#include "threads.h"

int is_running;
int city_cnt;
int sock;
pthread_mutex_t city_info_mutex;
pthread_mutex_t drone_mutex;
pthread_mutex_t ack_telemetria_mutex;
pthread_mutex_t ack_conclusao_mutex;
pthread_mutex_t sleep_mutex;
pthread_cond_t drone_cond;
pthread_cond_t ack_cond;
pthread_cond_t conclusao_cond;
pthread_cond_t sleep_cond;
pthread_cond_t telemetry_cond;
info_cidade_t* city_info;

static void sig_handler(int sig)
{
    is_running = 0;
    pthread_cond_broadcast(&ack_cond);
    pthread_cond_broadcast(&conclusao_cond);
    pthread_cond_broadcast(&drone_cond);
    pthread_cond_broadcast(&sleep_cond);
    pthread_cond_broadcast(&telemetry_cond);
}

int main(void)
{
    pthread_t threads[THREAD_CNT] = {};
    struct sockaddr_in serv_addr = {};
    int i;

    srand(time(NULL));
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sig_handler);

    if (
        pthread_mutex_init(&city_info_mutex, NULL) ||
        pthread_mutex_init(&drone_mutex, NULL) ||
        pthread_mutex_init(&ack_telemetria_mutex, NULL) ||
        pthread_mutex_init(&ack_conclusao_mutex, NULL) ||
        pthread_mutex_init(&sleep_mutex, NULL))
    {
        fprintf(stderr, "pthread_mutex_init() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        return 1;
    }

    if (
        pthread_cond_init(&drone_cond, NULL) ||
        pthread_cond_init(&ack_cond, NULL) ||
        pthread_cond_init(&sleep_cond, NULL) ||
        pthread_cond_init(&conclusao_cond, NULL) ||
        pthread_cond_init(&telemetry_cond, NULL))
    {
        fprintf(stderr, "pthread_cond_init() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        return 1;
    }

    if (get_info_from_file(FILENAME, &city_info, &city_cnt, NULL, NULL, NULL))
    {
        fprintf(stderr, "Failed to read data from the file, %s:%d\n", __func__, __LINE__);
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    if (1 != inet_pton(AF_INET, SERV_IP, &(serv_addr.sin_addr)))
    { 
        return 1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    printf("Conectado ao servidor %s:%d\n", SERV_IP, SERV_PORT);
    
    if (sock < 3)
    {
        fprintf(stderr, "socket() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        return 1;
    }

    is_running = 1;

    puts("Iniciando threads...");
    if (
        pthread_create(&threads[0], NULL, thread_monitoring_simulator, NULL) ||
        pthread_create(&threads[1], NULL, thread_telemetry_sender, (void *)&serv_addr) ||
        pthread_create(&threads[2], NULL, thread_msg_receiver, (void *)&serv_addr) ||
        pthread_create(&threads[3], NULL, thread_drone_team_action_simulator, (void *)&serv_addr))
    {
        return 1;
    }

    puts("Todas as threads iniciadas com sucesso\nPressione Ctrl+C para encerrar...\n");
    
    for (i = 0; i < THREAD_CNT; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&city_info_mutex);
    pthread_mutex_destroy(&drone_mutex);
    pthread_mutex_destroy(&ack_telemetria_mutex);
    pthread_mutex_destroy(&ack_conclusao_mutex);
    pthread_mutex_destroy(&sleep_mutex);
    pthread_cond_destroy(&drone_cond);
    pthread_cond_destroy(&ack_cond);
    pthread_cond_destroy(&conclusao_cond);
    pthread_cond_destroy(&sleep_cond);
    pthread_cond_destroy(&telemetry_cond);

    close(sock);
    free(city_info);

    return 0;
}
