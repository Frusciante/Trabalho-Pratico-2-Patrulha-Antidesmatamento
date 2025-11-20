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
        for (i = 0; i < city_cnt; ++i)     
        {
            if (rand() % 100 < 3)
            {
                city_info[i].status = ALERTA;
            }
        }
        pthread_mutex_unlock(&city_info_mutex);
        sleep(SLEEP_TIME);
    }

    return NULL;
}

void* thread_telemetry_sender(void* arg)
{
    struct timeval tv = {};
    int i, sock, cnt = 0, is_okay = 0;
    char send_buf[sizeof(header_t) + sizeof(payload_telemetria_t)];
    char recv_buf[sizeof(header_t) + sizeof(payload_ack_t)];
    header_t* header = (header_t*)send_buf;
    header_t* header_recv = (header_t*)recv_buf;
    payload_telemetria_t* payload = (payload_telemetria_t*)(send_buf + sizeof(header_t));
    payload_ack_t* payload_recv = (payload_ack_t*)recv_buf;
    struct sockaddr_in send_addr = {}, recv_addr = {};
    socklen_t recv_addr_len;

    header->tamanho = sizeof(payload_telemetria_t);
    header->tipo = MSG_TELEMETRIA;
    tv.tv_sec = UDP_TIMEOUT_CLIENT;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 3)
    {
        return NULL;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = SERV_PORT;
    if (1 != inet_pton(AF_INET, SERV_IP, &(send_addr.sin_addr)))
    {
        return NULL;
    }
    
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
        
        while (++cnt <= 3 || is_okay)
        {
            sendto(sock, (void *)send_buf, sizeof(send_buf), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
            if (sizeof(recv_buf) == recvfrom(sock, (void*)recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recv_addr, &recv_addr_len))
            {
                if (header_recv->tamanho == sizeof(payload_ack_t) && header_recv->tipo == MSG_ACK && payload_recv->status == ACK_TELEMETRIA)
                {
                    is_okay = 1;
                }
            }
        }
        is_okay = 0;
        cnt = 0;
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