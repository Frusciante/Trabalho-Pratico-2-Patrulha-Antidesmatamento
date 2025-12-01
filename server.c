#include "util.h"

static int is_running;

void sig_handler(int sig)
{
    is_running = 0;
}

static void update_info_cidade(const telemetria_t* const info_telemetria, info_cidade_t* info_cidade, int city_cnt)
{
    int i;
    if (!(info_telemetria && info_cidade))
    {
        return;
    }

    for (i = 0; i < city_cnt; i++)
    {
        info_cidade[i].id_cidade = info_telemetria[i].id_cidade;
        info_cidade[i].status = info_telemetria[i].status;
    }
}

/*
    A function fills 'output_arr' with shortest distances from 'vertex' to all other vertices.
*/
static int dijkstra(int vertex, const int** const adj_matrix, int* output_arr, int* visited_arr, int num_vertices)
{
    int i, j;
    int closest = INF;
    int closest_idx;
    int w;

    if (!(adj_matrix && output_arr && visited_arr) || vertex >= num_vertices)
    {
        fprintf(stderr, "Wrong parameter, %s:%d\n", __func__, __LINE__);
        return 1;
    }
    
    // initializing
    for (i = 0; i < num_vertices; ++i)
    {
        output_arr[i] = INF;
        visited_arr[i] = 0;
    }
    visited_arr[vertex] = 1;
    
    for (i = 0; i < num_vertices; ++i)
    {
        output_arr[i] = adj_matrix[vertex][i];
    }

    for (i = 0; i < num_vertices; i++)
    {
        for (j = 0; j < num_vertices; j++)
        {
            if (closest > output_arr[j] && visited_arr[j] == 0)
            {
                closest_idx = j;
                closest = output_arr[j];
            }
        }

        if (closest == INF)
        {
            break;
        }
        
        visited_arr[closest_idx] = 1;

        for (j = 0; j < num_vertices; j++)
        {
            w = adj_matrix[closest_idx][j];
            if (visited_arr[j] == 0 && w != INF)
            {
                // overflow test
                if (closest > INF - w)
                {
                    continue;
                }
             
                if (w + output_arr[closest_idx] < output_arr[j])
                {
                    output_arr[j] = w + output_arr[closest_idx];
                }
            }
        }
        
        closest = INF;
    }

    return 0;
}

int main(void)
{
    int** adj_matrix;
    info_cidade_t* city_info;
    int* dist_list;
    int* visited;
    int* capitals;
    int city_cnt;
    int capital_cnt;
    int i, j, min;
    int min_idx;
    int sock;
    struct sockaddr_in serv_addr = {};
    struct sockaddr_in clnt_addr = {};
    socklen_t clnt_addr_len;
    ssize_t size_received = 0;
    char recv_buf[512];
    char send_buf[512];
    size_t total_size;
    struct timeval tv = {1, 0};
    header_t* const header_ptr_send = (header_t*)send_buf;
    const header_t* const header_ptr_recv = (header_t*)recv_buf;
    payload_ack_t* const ack_ptr_send = (payload_ack_t*)(send_buf + sizeof(header_t));
    const payload_ack_t* const ack_ptr_recv = (payload_ack_t*)(recv_buf + sizeof(header_t));
    const payload_conclusao_t* const conclusao_ptr = (payload_conclusao_t*)(recv_buf + sizeof(header_t));
    payload_equipe_drone_t* const equipe_drone_ptr = (payload_equipe_drone_t*)(send_buf + sizeof(header_t));
    const payload_telemetria_t* const telemetria_ptr = (payload_telemetria_t*)(recv_buf + sizeof(header_t));
    
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sig_handler);
    srand(time(NULL));

    if (0 != get_info_from_file(FILENAME, &city_info, &city_cnt, &adj_matrix, &capitals, &capital_cnt))
    {
        fprintf(stderr, "Failed to read data from the file, %s:%d\n", __func__, __LINE__);
        return 1;
    }

    dist_list = (int*)malloc(city_cnt * sizeof(int));
    if (!dist_list)
    {
        fprintf(stderr, "malloc() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        FREE_SAFER(city_info);
        for (i = 0; i < city_cnt; ++i)
        {
            FREE_SAFER(adj_matrix[i]);
        }
        FREE_SAFER(adj_matrix);
        FREE_SAFER(capitals);
        return 1;
    }
    
    visited = (int*)malloc(city_cnt * sizeof(int));
    if (!visited)
    {
        fprintf(stderr, "malloc() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        FREE_SAFER(city_info);
        for (i = 0; i < city_cnt; ++i)
        {
            FREE_SAFER(adj_matrix[i]);
        }
        FREE_SAFER(adj_matrix);
        FREE_SAFER(capitals);
        FREE_SAFER(dist_list);
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 3)
    {
        fprintf(stderr, "socket() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        FREE_SAFER(city_info);
        for (i = 0; i < city_cnt; ++i)
        {
            FREE_SAFER(adj_matrix[i]);
        }
        FREE_SAFER(adj_matrix);
        FREE_SAFER(capitals);
        FREE_SAFER(dist_list);
        FREE_SAFER(visited);
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((short)SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (-1 == bind(sock, &serv_addr, sizeof(serv_addr)))
    {
        fprintf(stderr, "bind() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
        FREE_SAFER(city_info);
        for (i = 0; i < city_cnt; ++i)
        {
            FREE_SAFER(adj_matrix[i]);
        }
        FREE_SAFER(adj_matrix);
        FREE_SAFER(capitals);
        FREE_SAFER(dist_list);
        FREE_SAFER(visited);
        close(sock);
        return 1;
    }

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    is_running = 1;
    printf("Servidor escutando na porta %d...\n", SERV_PORT);
    while (is_running)
    {
        clnt_addr_len = sizeof(clnt_addr);
        size_received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
        if (size_received < (ssize_t)(sizeof(header_t)))
        {
            continue;
        }

        if (size_received != header_ptr_recv->tamanho + (ssize_t)(sizeof(header_t)))
        {
            continue;
        }

        switch (header_ptr_recv->tipo)
        {
        case MSG_TELEMETRIA:
            printf("[TELEMETRIA RECEBIDA]\nTotal de cidades monitoradas: %d\n", telemetria_ptr->total);
            
            if (telemetria_ptr->total > city_cnt)
            {
                update_info_cidade(telemetria_ptr->dados, city_info, city_cnt);
            }
            else
            {
                update_info_cidade(telemetria_ptr->dados, city_info, telemetria_ptr->total);
            }
            for (i = 0; i < city_cnt; ++i)
            {
                if (city_info[i].status == ALERTA)
                {
                    printf("ALERTA: %s (ID=%d)\n", city_info[i].nome_cidade, city_info[i].id_cidade);
                }
            }
            
            total_size = sizeof(header_t) + sizeof(payload_ack_t);
            header_ptr_send->tamanho = (uint16_t)(sizeof(payload_ack_t));
            header_ptr_send->tipo = MSG_ACK;
            ack_ptr_send->status = ACK_TELEMETRIA;

            if (-1 == sendto(sock, (void*)send_buf, total_size, 0, (struct sockaddr *)&clnt_addr, clnt_addr_len))
            {
                fprintf(stderr, "sendto() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
                continue;
            }
            printf("-> ACK enviado (tipo=%d)\n", ack_ptr_send->status);

            for (i = 0; i < city_cnt; ++i)
            {
                if (city_info[i].status == ALERTA && city_info[i].equipe_atuando == 0)
                {
                    printf("[DESPACHANDO DRONES]\nCidade em alerta: %s (ID=%d)\n", city_info[i].nome_cidade, i);
                    dijkstra(i, (const int** const)adj_matrix, dist_list, visited, city_cnt);
                    min = 0x7FFFFFFF;
                    min_idx = -1;
                    for (j = 0; j < capital_cnt; j++)
                    {
                        if (min > dist_list[capitals[j]] && city_info[capitals[j]].drone_disponivel == 1)
                        {
                            min = dist_list[capitals[j]];
                            min_idx = capitals[j];
                        }
                    }
                    if (min == 0x7FFFFFFF || min_idx == -1)
                    {
                        // exception handling
                        fprintf(stderr, "Failed to find the available drone team, %s:%d\n", __func__, __LINE__);
                        continue;
                    }

                    printf("-> Dijkstra: capital %s (ID=%d) selecionada, distância = %d km\n", city_info[min_idx].nome_cidade, min_idx, min);

                    total_size = sizeof(header_t) + sizeof(payload_equipe_drone_t);
                    header_ptr_send->tamanho = (uint16_t)(sizeof(payload_equipe_drone_t));
                    header_ptr_send->tipo = MSG_EQUIPE_DRONE;
                    equipe_drone_ptr->id_cidade = i;
                    equipe_drone_ptr->id_equipe = min_idx;
                    if (-1 == sendto(sock, send_buf, total_size, 0, &clnt_addr, clnt_addr_len))
                    {
                        fprintf(stderr, "sendto() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
                        continue;
                    }
                    
                    printf("-> Ordem enviada: Equipe %s (ID=%d) -> Cidade %s (ID=%d)\n", city_info[min_idx].nome_cidade, min_idx, city_info[i].nome_cidade, i);

                    size_received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&clnt_addr, &clnt_addr_len);
                    if (size_received == (ssize_t)(sizeof(header_t)) + (ssize_t)(sizeof(payload_ack_t)) && header_ptr_recv->tipo == MSG_ACK)
                    {
                        if (ack_ptr_recv->status == ACK_EQUIPE_DRONE)
                        {
                            printf("[ACK RECEBIDO]\nCliente confirmou recebimento de ordem de drone para %s\n", city_info[i].nome_cidade);
                            city_info[i].equipe_atuando = 1;
                            city_info[min_idx].drone_disponivel = IS_NOT_AVAILABLE;
                        }
                    }
                }
            }
            break;
        case MSG_CONCLUSAO:
            city_info[conclusao_ptr->id_cidade].equipe_atuando = 0;
            city_info[conclusao_ptr->id_equipe].drone_disponivel = IS_FREE;
            puts("MISSÃO CONCLUÍDA");
            total_size = sizeof(header_t) + sizeof(payload_ack_t);
            header_ptr_send->tamanho = (ssize_t)(sizeof(payload_ack_t));
            header_ptr_send->tipo = MSG_ACK;
            ack_ptr_send->status = ACK_CONCLUSAO;
            printf("Cidade atendida: %s (ID=%d)\nEquipe: %s (ID=%d)\n-> Equipe %s liberado para novas missões\n", city_info[conclusao_ptr->id_cidade].nome_cidade, conclusao_ptr->id_cidade, city_info[conclusao_ptr->id_equipe].nome_cidade, conclusao_ptr->id_equipe, city_info[conclusao_ptr->id_equipe].nome_cidade);
            if (-1 == sendto(sock, send_buf, total_size, 0, (struct sockaddr *)&clnt_addr, clnt_addr_len))
            {
                fprintf(stderr, "sendto() error (%s), %s:%d\n", strerror(errno), __func__, __LINE__);
            }
            else
            {
                printf("-> ACK enviado (tipo=%d)\n", ack_ptr_send->status);
            }
            break;
        }
    }

    close(sock);
    FREE_SAFER(visited);
    FREE_SAFER(dist_list);
    FREE_SAFER(city_info);
    FREE_SAFER(capitals);
    for (i = 0; i < city_cnt; ++i)
    {
        FREE_SAFER(adj_matrix[i]);
    }
    FREE_SAFER(adj_matrix);

    return 0;
}
