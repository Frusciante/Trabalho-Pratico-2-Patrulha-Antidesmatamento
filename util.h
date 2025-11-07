#ifndef _UTIL_H
#define _UTIL_H

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

#define INF 0x7FFFFFFF
#define CITY_NAME_LEN 30
#define FILENAME "grafo_amazonia_legal.txt" 


typedef enum
{
    MSG_TELEMETRIA = 1,
    MSG_ACK,
    MSG_EQUIPE_DE_DRONES,
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
    OK,
    ALERTA
} telemetria_status_e;

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
    int eh_capital;
    char nome_cidade[CITY_NAME_LEN]; 
} info_cidade_t;

void remove_whitespace(char* str);
int is_valid_int(const char* const str);
int get_info_from_file(const char* const filename, info_cidade_t** city_info_ptr, int* city_cnt, int*** adj_matrix_ptr);

#endif
