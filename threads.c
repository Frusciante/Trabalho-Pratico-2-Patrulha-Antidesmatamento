#include "threads.h"

extern info_cidade_t* city_info;
extern pthread_mutex_t city_info_mutex;
extern pthread_mutex_t ack_telemetria_mutex;
extern pthread_mutex_t ack_conclusao_mutex;
extern pthread_mutex_t drone_mutex;
extern pthread_cond_t ack_cond;
extern pthread_cond_t drone_cond;
extern pthread_cond_t conclusao_cond;
extern int city_cnt;
extern int sock;
extern int is_running;

static int ack_telemetria_received;
static int ack_conclusao_received;
static event_queue* events_head = NULL;
static event_queue* events_tail = NULL;

void* thread_monitoring_simulator(void* arg)
{
    int i;

    while (is_running)
    {
        pthread_mutex_lock(&city_info_mutex); 
        for (i = 0; i < city_cnt; ++i)     
        {
            city_info[i].status = (rand() % 100 < 3) ? ALERTA : OK;
        }
        pthread_mutex_unlock(&city_info_mutex);
        
        if (is_running)
        {
            sleep(SLEEP_TIME);
        }
    }

    return NULL;
}

void* thread_telemetry_sender(void* arg)
{
    int i;
    char send_buf[sizeof(header_t) + sizeof(payload_telemetria_t)];
    header_t* header = (header_t*)send_buf;
    payload_telemetria_t* payload = (payload_telemetria_t*)(send_buf + sizeof(header_t));
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)arg;
    
    header->tamanho = sizeof(payload_telemetria_t);
    header->tipo = MSG_TELEMETRIA;

    while (is_running)
    {
        pthread_mutex_lock(&city_info_mutex);
        payload->total = city_cnt;
        for (i = 0; i < city_cnt; ++i)
        {
            payload->dados[i].id_cidade = city_info[i].id_cidade;
            payload->dados[i].status = city_info[i].status;
        }
        pthread_mutex_unlock(&city_info_mutex);
        
        sendto_with_retry(sock, send_buf, sizeof(send_buf), (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in), &ack_telemetria_mutex, &ack_cond, &ack_telemetria_received, UDP_TIMEOUT_CLIENT, &is_running);
        
        if (is_running)
        {
            sleep(SLEEP_TIME);
        }
    }

    return NULL;
}

void* thread_msg_receiver(void* arg)
{
    char send_buf[sizeof(header_t) + sizeof(payload_equipe_drone_t)] = {};
    char recv_buf[sizeof(header_t) + sizeof(payload_ack_t)] = {};
    struct sockaddr_in recv_addr = {};
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)arg;
    socklen_t recv_addr_len;
    header_t* header_send = (header_t*)send_buf;
    header_t* header_recv = (header_t*)recv_buf;
    payload_ack_t* payload_send = (payload_ack_t*)(send_buf + sizeof(header_t));
    payload_equipe_drone_t* payload_recv = (payload_equipe_drone_t*)(recv_buf + sizeof(header_t));
    event_queue* temp_event = NULL;
    struct timeval tv = {1, 0};
    
    if (0 > setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
    {
        // error handling 
    }

    while (is_running)
    {
        if (sizeof(header_t) < recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recv_addr, &recv_addr_len))
        {
            continue;
        }

        switch (header_recv->tipo)
        {
        case MSG_ACK:
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
                // queue에 쌓기
                temp_event = (event_queue*)malloc(sizeof(event_queue));
                if (!temp_event)
                {
                    continue;
                }
                temp_event->id_cidade = payload_recv->id_cidade;
                temp_event->id_equipe = payload_recv->id_equipe;
                temp_event->next = NULL;
                
                pthread_mutex_lock(&drone_mutex);
                if (events_head == NULL)
                {
                    events_head = events_tail = temp_event;
                }
                else
                {
                    events_tail->next = temp_event;
                    events_tail = temp_event;
                }
                pthread_cond_signal(&drone_cond);
                pthread_mutex_unlock(&drone_mutex);

                header_send->tamanho = sizeof(payload_ack_t);
                header_send->tipo = MSG_ACK;
                payload_send->status = ACK_EQUIPE_DRONE;

                sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in));
            }
            break;
        }
    }
    
    return NULL;
}

void* thread_drone_team_action_simulator(void* arg)
{
    char send_buf[sizeof(header_t) + sizeof(payload_conclusao_t)];
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)arg;
    payload_conclusao_t* payload_send = (payload_conclusao_t*)(send_buf + sizeof(header_t)); 
    struct event_queue* temp = NULL;
    
    while (is_running)
    {
        pthread_mutex_lock(&drone_mutex);
        while (is_running && events_head == NULL)
        {
            pthread_cond_wait(&drone_cond, &drone_mutex);
        }

        if (!is_running)
        {
            pthread_mutex_unlock(&drone_mutex);
            break;
        }

        payload_send->id_cidade = events_head->id_cidade;
        payload_send->id_equipe = events_head->id_equipe;
        temp = events_head;
        events_head = events_head->next;
        free((void*)temp);
        if (events_head == NULL)
        {
            events_tail = NULL;
        }
        pthread_mutex_unlock(&drone_mutex);
        
        sleep((rand() % (SLEEP_TIME - 1)) + 1);

        
        sendto_with_retry(sock, send_buf, sizeof(send_buf), (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in), &ack_conclusao_mutex, &conclusao_cond, &ack_conclusao_received, UDP_TIMEOUT_CLIENT, &is_running);

    }
    
    while (events_head != NULL)
    {
        temp = events_head;
        events_head = events_head->next;
        free((void*)temp);
    }

    return NULL;
}