#include "threads.h"

extern info_cidade_t* city_info;
extern pthread_mutex_t city_info_mutex;
extern pthread_mutex_t ack_mutex;
extern pthread_mutex_t drone_mutex;
extern pthread_cond_t ack_cond;
extern int city_cnt;
extern int sock;
extern int is_running;

static int city_id = -1;
static int drone_team_id = -1;
static int ack_received;

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
    int i, cnt = 0, is_okay = 0;
    char send_buf[sizeof(header_t) + sizeof(payload_telemetria_t)];
    char recv_buf[sizeof(header_t) + sizeof(payload_ack_t)];
    header_t* header = (header_t*)send_buf;
    header_t* header_recv = (header_t*)recv_buf;
    payload_telemetria_t* payload = (payload_telemetria_t*)(send_buf + sizeof(header_t));
    payload_ack_t* payload_recv = (payload_ack_t*)(recv_buf + sizeof(header_t));
    struct sockaddr_in recv_addr = {};
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)arg;
    socklen_t recv_addr_len;
    struct timespec ts = {};
    int check_timeout = 0;
    
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
        
        while (is_running && (++cnt <= 3 && is_okay == 0))
        {
            sendto(sock, (void *)send_buf, sizeof(send_buf), 0, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in));
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += SLEEP_TIME;
            check_timeout = 0;
            pthread_mutex_lock(&ack_mutex);
            while (ack_received == 0 && check_timeout != ETIMEDOUT)
            {
                check_timeout = pthread_cond_timedwait(&ack_cond, &ack_mutex, &ts);
            }
            if (ack_received)
            {
                is_okay = 1;
            }
            pthread_mutex_unlock(&ack_mutex);
        }
        is_okay = 0;
        cnt = 0;
        
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
    
    header_send->tamanho = sizeof(payload_ack_t);
    header_send->tipo = MSG_ACK;
    payload_send->status = ACK_EQUIPE_DRONE;

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
                    
                case ACK_TELEMETRIA:
                    pthread_mutex_lock(&ack_mutex);
                    ack_received = 1;
                    pthread_cond_signal(&ack_cond);
                    pthread_mutex_unlock(&ack_mutex);
                }
            }
            break;
        case MSG_EQUIPE_DRONE:
            if (header_recv->tamanho == sizeof(payload_equipe_drone_t))
            {
                city_id = payload_recv->id_cidade;
                drone_team_id = payload_recv->id_equipe;
                sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in));
            }
            break;
        }
    }
    
    return NULL;
}

void* thread_drone_team_action_simulator(void* arg)
{
    int i;
    char send_buf[sizeof(header_t) + sizeof(payload_conclusao_t)];
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)arg;
    header_t* header_send = (header_t*)send_buf;
    payload_conclusao_t* payload_send = (payload_conclusao_t*)(send_buf + sizeof(header_t));

    while (is_running)
    {
        for (i = 0; i < city_cnt; ++i)
        {
            if (city_info[i].status == ALERTA)
            {
                sleep((rand() % (SLEEP_TIME - 1)) + 1);
                pthread_mutex_lock(&city_info_mutex);
                city_info[i].status = OK;
                pthread_mutex_unlock(&city_info_mutex);

                payload_send->id_cidade = city_info[i].id_cidade;
                payload_send->id_equipe = city_info[i].equipe_atuando;
                
                sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in));
            }
        }
    }

    return NULL;
}