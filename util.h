#include <stdint.h>
#define INF 0x7FFFFFFF

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
    int id_cidade;  // identificador do vertice
    int status;     // 0 = OK, 1 = ALERTA
} telemetria_t;

typedef struct
{
    int total;              // numero de cidades monitoradas
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
