#include "threads.h"

extern info_cidade_t* city_info;
extern pthread_mutex_t city_info_mutex;
extern pthread_mutex_t ack_telemetria_mutex;
extern pthread_mutex_t ack_conclusao_mutex;
extern pthread_mutex_t event_mutex;
extern pthread_mutex_t sleep_mutex;
extern pthread_cond_t ack_cond;
extern pthread_cond_t event_cond;
extern pthread_cond_t conclusao_cond;
extern pthread_cond_t sleep_cond;
extern pthread_cond_t telemetry_cond;
extern int city_cnt;
extern int sock;
extern int is_running;

static int ack_telemetria_received;
static int ack_conclusao_received;
static int is_telemetry_ready;
static event_queue events = {NULL, NULL}; 

void* thread_monitoring_simulator(void* arg)
{
    int i;

    puts("[Thread Monitoramento] Iniciada");

    while (is_running)
    {
        pthread_mutex_lock(&city_info_mutex); 
        for (i = 0; i < city_cnt; ++i)     
        {
            city_info[i].status = (rand() % 100 < 3) ? ALERTA : OK;
        }
        is_telemetry_ready = 1;
        pthread_cond_signal(&telemetry_cond);
        pthread_mutex_unlock(&city_info_mutex);
        
        if (is_running)
        {
            sleep_to_be_awaken(SLEEP_TIME, &is_running, &sleep_mutex, &sleep_cond);
        }
    }

    return NULL;
}
void* thread_telemetry_sender(void* arg)
{
    int i;
    char send_buf[sizeof(header_t) + sizeof(payload_telemetria_t)];
    header_t* const header = (header_t*)send_buf;
    payload_telemetria_t* const payload = (payload_telemetria_t*)(send_buf + sizeof(header_t));
    const struct sockaddr_in* const serv_addr = (struct sockaddr_in*)arg;

    puts("[Thread Telemetria] Iniciada");
    
    if (!arg)
    {
        fprintf(stderr, "Wrong thread argument, %s:%d\n", __func__, __LINE__);
        return (void*)1;
    }

    while (is_running)
    {
        pthread_mutex_lock(&city_info_mutex);

        while (!is_telemetry_ready && is_running)
        {
            pthread_cond_wait(&telemetry_cond, &city_info_mutex);
        }

        if (!is_running)
        {
            pthread_mutex_unlock(&city_info_mutex);
            break;
        }

        payload->total = city_cnt;
        for (i = 0; i < city_cnt; ++i)
        {
            payload->dados[i].id_cidade = city_info[i].id_cidade;
            payload->dados[i].status = city_info[i].status;
        }
        is_telemetry_ready = 0;
        pthread_mutex_unlock(&city_info_mutex);

        printf("[ENVIANDO TELEMETRIA]\nTotal de cidades: %d\n", city_cnt); 
        for (i = 0; i < city_cnt; ++i)
        {
            if (payload->dados[i].status == ALERTA)
            {
                printf("ALERTA: %s (ID=%d)\n", city_info[i].nome_cidade, i);        
            }
        }
        header->tamanho = sizeof(payload_telemetria_t);
        header->tipo = MSG_TELEMETRIA;
        sendto_with_retry(sock, send_buf, sizeof(send_buf), (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in), "-> Telemetria enviada", &ack_telemetria_mutex, &ack_cond, &ack_telemetria_received, UDP_TIMEOUT_CLIENT, &is_running);

        // initialization to prevent side effect
        memset(send_buf, 0, sizeof(send_buf));
    }

    return NULL;
}

/*
    To prevent the race condition of message among threads, 'recvfrom()' is only called in this function.
    When the message arrives, this function parse the data and calls 'pthread_cond_signal()' to wake up designated thread.
*/
void* thread_msg_receiver(void* arg)
{
    char send_buf[sizeof(header_t) + sizeof(payload_ack_t)] = {};
    char recv_buf[sizeof(header_t) + sizeof(payload_equipe_drone_t)] = {};
    struct sockaddr_in recv_addr = {};
    const struct sockaddr_in* const serv_addr = (struct sockaddr_in*)arg;
    socklen_t recv_addr_len;
    header_t* const header_send = (header_t*)send_buf;
    const header_t* const header_recv = (header_t*)recv_buf;
    payload_ack_t* const payload_send = (payload_ack_t*)(send_buf + sizeof(header_t));
    const payload_equipe_drone_t* const payload_recv = (payload_equipe_drone_t*)(recv_buf + sizeof(header_t));
    const struct timeval tv = {1, 0};
    
    puts("[Thread Recepção Drones] Iniciada");

    if (!arg)
    {
        fprintf(stderr, "Wrong thread argument, %s:%d\n", __func__, __LINE__);
        return (void*)1;
    }
    
    if (0 > setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
    {
        fprintf(stderr, "setsockopt() error, %s:%d", __func__, __LINE__);
    }

    while (is_running)
    {
        if (sizeof(header_t) > recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recv_addr, &recv_addr_len))
        {
            continue;
        }

        switch (header_recv->tipo)
        {
        case MSG_ACK:
            puts("ACK recebido do servidor");
            if (header_recv->tamanho == sizeof(payload_ack_t))
            {
                switch (((payload_ack_t *)(recv_buf + sizeof(header_t)))->status)
                {
                case ACK_CONCLUSAO:
                    pthread_mutex_lock(&ack_conclusao_mutex);
                    ack_conclusao_received = 1;
                    pthread_cond_signal(&conclusao_cond);
                    pthread_mutex_unlock(&ack_conclusao_mutex);
                    break;
                case ACK_TELEMETRIA:
                    pthread_mutex_lock(&ack_telemetria_mutex);
                    ack_telemetria_received = 1;
                    pthread_cond_signal(&ack_cond);
                    pthread_mutex_unlock(&ack_telemetria_mutex);
                    break;
                }
            }
            break;
        case MSG_EQUIPE_DRONE:
            if (header_recv->tamanho == sizeof(payload_equipe_drone_t))
            {
                printf("[ORDEM DE DRONE RECEBIDA]\nCidade: %s (ID=%d)\nEquipe: %s (ID=%d)\n", city_info[payload_recv->id_cidade].nome_cidade, payload_recv->id_cidade, city_info[payload_recv->id_equipe].nome_cidade, payload_recv->id_equipe);

                pthread_mutex_lock(&event_mutex);
                enqueue(&events, payload_recv->id_cidade, payload_recv->id_equipe);
                pthread_cond_signal(&event_cond);
                pthread_mutex_unlock(&event_mutex);

                header_send->tamanho = sizeof(payload_ack_t);
                header_send->tipo = MSG_ACK;
                payload_send->status = ACK_EQUIPE_DRONE;

                sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in));
            }
            break;
        }

        // initialization to prevent side effect
        memset(send_buf, 0, sizeof(send_buf));
        memset(recv_buf, 0, sizeof(recv_buf));
    }
    
    return NULL;
}

void* thread_drone_team_action_simulator(void* arg)
{
    char send_buf[sizeof(header_t) + sizeof(payload_conclusao_t)];
    const struct sockaddr_in* const serv_addr = (struct sockaddr_in*)arg;
    header_t* const header_send = (header_t*)send_buf;
    payload_conclusao_t* const payload_send = (payload_conclusao_t*)(send_buf + sizeof(header_t)); 
    struct event_node event_buf = {};
    int sleep_time;

    puts("[Thread Simulação Drones] Iniciada");
    
    if (!arg)
    {
        fprintf(stderr, "Wrong thread argument, %s:%d\n", __func__, __LINE__);
        return (void*)1;
    }
    
    while (is_running)
    {
        pthread_mutex_lock(&event_mutex);
        while (is_running && events.head == NULL)
        {
            pthread_cond_wait(&event_cond, &event_mutex);
        }

        if (!is_running)
        {
            pthread_mutex_unlock(&event_mutex);
            break;
        }
        
        dequeue(&events, &event_buf);
        pthread_mutex_unlock(&event_mutex);
        
        payload_send->id_cidade = event_buf.id_cidade;
        payload_send->id_equipe = event_buf.id_equipe;
        header_send->tamanho = sizeof(payload_conclusao_t);
        header_send->tipo = MSG_CONCLUSAO;
        sleep_time = (rand() % (SLEEP_TIME - 1)) + 1;
        printf("Equipe %s atuando em %s\nTempo estimado: %d segundos\n...\n", city_info[payload_send->id_equipe].nome_cidade, city_info[payload_send->id_cidade].nome_cidade, sleep_time);
        sleep_to_be_awaken(sleep_time, &is_running, &sleep_mutex, &sleep_cond);
        puts("Missão concluída!");
        sendto_with_retry(sock, send_buf, sizeof(send_buf), (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in), "-> Conclusão enviada ao servidor", &ack_conclusao_mutex, &conclusao_cond, &ack_conclusao_received, UDP_TIMEOUT_CLIENT, &is_running);
        
        // initialization to prevent side effect
        memset(send_buf, 0, sizeof(send_buf));
    }
    
    free_queue(&events);

    return NULL;
}
