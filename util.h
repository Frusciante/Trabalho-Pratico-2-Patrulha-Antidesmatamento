#ifndef _UTIL_H
#define _UTIL_H
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#define INF 0x7FFFFFFF
#define CITY_NAME_LEN 64 
#define UDP_TIMEOUT_CLIENT 5 
#define UDP_TIMEOUT_SERVER 1 
#define SLEEP_TIME 30
#define NUM_RETRY 3
#define SERV_PORT 8080
#define SERV_IP "127.0.0.1" 
#define FILENAME "grafo_amazonia_legal.txt" 
#define FREE_SAFER(p) do { if((p)) { free((p)); (p) = NULL; } } while(0)

typedef enum
{
    MSG_TELEMETRIA = 1,
    MSG_ACK,
    MSG_EQUIPE_DRONE,
    MSG_CONCLUSAO
} message_type_e;

typedef enum 
{
    ACK_TELEMETRIA,
    ACK_EQUIPE_DRONE,
    ACK_CONCLUSAO
} ack_status_e;

typedef enum 
{
    NORMAL,
    ALERTA
} telemetria_status_e;

typedef enum
{
    IS_NOT_CAPITAL = -1,
    IS_NOT_AVAILABLE,
    IS_FREE
} city_status_e;

typedef struct
{
    // Tipo da mensagem
    // (1 = telemetria, 2 = ACK, 3 = equipe de drones, 4 = conclusão
    uint16_t tipo;
    // tamanho do payload em bytes
    uint16_t tamanho;
} header_t;

typedef struct
{
    int id_cidade;  // identificador do vertice
    int status;     // 0 = OK, 1 = ALERTA
} telemetria_t;

typedef struct
{
    int total;              // número de cidades monitoradas
    telemetria_t dados[50]; // lista de (id_cidade, status)
} payload_telemetria_t;

typedef struct
{
    int status; // 0 = ACK_TELEMETRIA, 1 = ACK_EQUIPE_DRONE, 2 = ACK_CONCLUSAO
} payload_ack_t;

typedef struct
{
    int id_cidade;  // cidade onde o alerta foi detectado
    int id_equipe;  // equipe de drones designada
} payload_equipe_drone_t;

typedef struct
{
    int id_cidade;  // cidade atendida
    int id_equipe;  // equipe que atuou
} payload_conclusao_t;

typedef struct
{
    int id_cidade;
    int drone_disponivel; 
    char nome_cidade[CITY_NAME_LEN]; 
    int status;
    int equipe_atuando;
} info_cidade_t;

typedef struct event_node
{
    int id_cidade;
    int id_equipe;
    struct event_node* next;
} event_node;

typedef struct event_queue
{
    event_node* head;
    event_node* tail;
} event_queue;

void remove_whitespace(char* str);
int is_valid_int(const char* const str);
int get_info_from_file(const char* const filename, info_cidade_t** city_info_ptr, int* city_cnt, int*** adj_matrix_ptr, int** capitals_ptr, int* capital_cnt);
int sendto_with_retry(int sock, const void* buf, size_t len, struct sockaddr* addr, socklen_t addr_len, const char* message, pthread_mutex_t* mutex, pthread_cond_t* cond, int* ack_flag, int timeout, int* is_running_ptr);
void sleep_to_be_awaken(int secs, int* is_running_ptr, pthread_mutex_t* mutex,  pthread_cond_t* cond);
int enqueue(event_queue* queue, int id_cidade, int id_equipe);
int dequeue(event_queue* queue, event_node* output);
void free_queue(event_queue* queue);

#endif
