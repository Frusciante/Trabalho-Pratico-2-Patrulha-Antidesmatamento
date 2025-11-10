#include "util.h"

static int is_running;

void sig_handler(int sig)
{
    is_running = 0;
}

static void update_info_cidade(const telemetria_t* const info_telemetria, info_cidade_t* info_cidade, int city_cnt)
{
    int i, id;
    if (!(info_telemetria && info_cidade))
    {
        return;
    }

    for (i = 0; i < city_cnt; i++)
    {
        id = info_telemetria[i].id_cidade;
        info_cidade[id].status = info_telemetria[i].status;
    }
}

/*
    A function fills 'output_arr' with shortest distances from 'vertex' to all other vertices.
*/
static int dijkstra(int vertex, const unsigned int** const adj_matrix, unsigned int* output_arr, int* visited_arr, int num_vertices)
{
    int i, j;
    unsigned int closest = INF;
    unsigned int closest_idx;
    int w;

    if (!(adj_matrix && output_arr && visited_arr) || vertex >= num_vertices)
    {
        return 1;
    }
    
    memset(visited_arr, 0, num_vertices * sizeof(int));
    memset(output_arr, INF, num_vertices * sizeof(int));
    visited_arr[vertex] = 1;
    
    for (i = 0; i < num_vertices; i++)
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
    unsigned int* dist_list;
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
    char recv_buf[512];
    char send_buf[512];
    size_t total_size;
    struct timeval tv = {};
    header_t* header_ptr = NULL;
    payload_ack_t* ack = NULL;
    ssize_t size_received = 0;
    payload_ack_t* ack_ptr;
    payload_conclusao_t* conclusao_ptr;
    payload_equipe_drone_t* equipte_drone_ptr;
    payload_telemetria_t* telemetria_ptr;
    
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sig_handler);

    get_info_from_file(FILENAME, &city_info, &city_cnt, &adj_matrix, &capitals, &capital_cnt);
    dist_list = (unsigned int*)malloc(city_cnt * sizeof(unsigned int));
    if (!dist_list)
    {
        return 1;
    }
    
    visited = (int*)malloc(city_cnt * sizeof(int));
    if (!visited)
    {
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket < 3)
    {
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t)SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (-1 == bind(sock, &serv_addr, sizeof(serv_addr)))
    {
        close(sock);
        return 1;
    }

    while (is_running)
    {
        tv.tv_sec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        size_received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
        if (size_received < sizeof(header_t))
        {
            continue;
        }

        header_ptr = (header_t*)recv_buf;
        if (size_received != header_ptr->tamanho + sizeof(header_t))
        {
            continue;
        }

        switch (header_ptr->tipo)
        {
        case MSG_TELEMETRIA:
            update_info_cidade((telemetria_t *)(recv_buf + sizeof(header_t)), city_info, city_cnt);
            total_size = sizeof(header_t) + sizeof(payload_ack_t);
            header_ptr = (header_t *)send_buf;
            header_ptr->tamanho = sizeof(payload_ack_t);
            header_ptr->tipo = MSG_ACK;
            ack_ptr = ((payload_ack_t *)(send_buf + sizeof(header_t)));
            ack_ptr->status = ACK_TELEMETRIA;

            if (-1 == sendto(sock, send_buf, total_size, 0, (struct sockaddr *)&clnt_addr, clnt_addr_len))
            {
                // error handling
            }

            for (i = 0; i < city_cnt; i++)
            {
                if (city_info[i].status == ALERTA && city_info[i].equipe_atuando == 0)
                {
                    dijkstra(i, adj_matrix, dist_list, visited, city_cnt);
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
                        continue;
                    }
                    total_size = sizeof(header_t) + sizeof(payload_equipe_drone_t);
                    header_ptr = ((header_t *)send_buf);
                    header_ptr->tamanho = sizeof(payload_equipe_drone_t);
                    header_ptr->tipo = MSG_EQUIPE_DE_DRONES;
                    equipte_drone_ptr = (payload_equipe_drone_t *)(send_buf + sizeof(header_t));
                    equipte_drone_ptr->id_cidade = i;
                    equipte_drone_ptr->id_equipe = min_idx;
                    if (-1 == sendto(sock, send_buf, total_size, 0, &clnt_addr, clnt_addr_len))
                    {
                        // error handling
                    }

                    tv.tv_sec = UDP_TIMEOUT_SERVER;
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                    size_received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&clnt_addr, &clnt_addr_len);
                    header_ptr = (header_t*)recv_buf;
                    if (size_received > sizeof(header_t) && (size_received == header_ptr->tamanho + sizeof(header_t)) && header_ptr->tipo == MSG_ACK)
                    {
                        ack_ptr = (payload_ack_t*)(recv_buf + sizeof(header_t));
                        if (ack_ptr->status == ACK_EQUIPE_DRONE)
                        {
                            city_info[i].evento_timestamp = time(NULL);
                            city_info[i].equipe_atuando = 1;
                            city_info[min_idx].drone_disponivel = 0;
                        }
                    }
                }
            }
            break;
        case MSG_CONCLUSAO:
            conclusao_ptr = (payload_conclusao_t *)(recv_buf + sizeof(header_t));
            city_info[conclusao_ptr->id_cidade].evento_timestamp = 0;
            city_info[conclusao_ptr->id_equipe].drone_disponivel = 1;
            total_size = sizeof(header_t) + sizeof(payload_ack_t);
            header_ptr = (header_t *)send_buf;
            header_ptr->tamanho = sizeof(payload_ack_t);
            header_ptr->tipo = MSG_ACK;
            ack_ptr = ((payload_ack_t *)(send_buf + sizeof(header_t)));
            ack_ptr->status = ACK_CONCLUSAO;
            if (-1 == sendto(sock, send_buf, total_size, 0, (struct sockaddr *)&clnt_addr, clnt_addr_len))
            {
                // error handling
            }
            break;
        }
    }

    free(city_info);
    for (i = 0; i < city_cnt; i++)
    {
        free(adj_matrix[i]);
    }
    free(adj_matrix);

    return 0;
}
