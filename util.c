#include "util.h"

void remove_whitespace(char* str)
{
    char* start = str;
    char* end;
    size_t len;
    if (!str)
    {
        return;
    }

    len = strlen(start);
    if (len < 1)
    {
        return;
    }
    
    end = str + (len - 1);
    
    while (end > start && isspace((char)*end))
    {
        end--;
    }

    *(end + 1) = 0;

    while (*start && isspace((char)*start))
    {
        start++;
    }

    if (start != str)
    {
        memmove(str, start, len + 1);
    }
}

int is_valid_int(const char* const str)
{
    const char* str_iter = str;
    if (!str || *str == '\0')
    {
        return 0;
    }

    if (*str_iter == '-')
    {
        str_iter++;
    }

    while (*str_iter != '\0')
    {
        if (*str_iter < '0' && *str_iter > '9')
        {
            return 0;
        }
        str_iter++;
    }
    
    return 1;
}

/*
    Open and read information from the file.
    if parameters, 'capitals_ptr' or 'city_info_ptr' are NULL, this function will not fill these pointers. 
*/
int get_info_from_file(const char* const filename, info_cidade_t** city_info_ptr, int* city_cnt, int*** adj_matrix_ptr, int** capitals_ptr, int* capital_cnt_ptr)
{
    FILE* fp = NULL;
    char buffer[256];
    char* save_ptr = buffer;
    char* num_city_ptr = NULL;
    char* num_edge_ptr = NULL;
    char* type_start = NULL;
    char* v1_ptr = NULL;
    char* v2_ptr = NULL;
    char* w_ptr = NULL;
    int len;
    int type;
    int id;
    int edge_cnt;
    int city_cnt_temp;
    int cnt = 0;
    int name_len;
    int v1, v2, w;
    int i, j;
    int capital_cnt = 0;
    int* capital_buf;

    if (!(filename && city_info_ptr && city_cnt))
    {
        fprintf(stderr, "Wrong parameter : %s:%d\n", __func__, __LINE__);
        return 1;
    }

    fp = fopen(filename, "rt"); 
    if (!fp)
    {
        fprintf(stderr, "fopen() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
        return 1;
    }

    fgets(buffer, sizeof(buffer), fp);
    num_city_ptr = strtok_r(buffer, " \t", &save_ptr);
    num_edge_ptr = strtok_r(NULL, " \t", &save_ptr);
    
    city_cnt_temp = atoi(num_city_ptr);
    edge_cnt = atoi(num_edge_ptr);
    
    if (city_cnt_temp <= 0 || edge_cnt <= 0)
    {
        fprintf(stderr, "Wrong city count or edge count\ncity count: %d\nedge count: %d\n", city_cnt_temp, edge_cnt);
        fclose(fp);
        return 1;
    }
    
    *city_cnt = city_cnt_temp;

    *city_info_ptr = (info_cidade_t*)calloc(city_cnt_temp, sizeof(info_cidade_t));
    if (!(*city_info_ptr))
    {
        fprintf(stderr, "calloc() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
        fclose(fp);
        return 1;
    }

    capital_buf = (int*)calloc(city_cnt_temp, sizeof(int));
    if (!capital_buf)
    {
        fprintf(stderr, "calloc() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
        fclose(fp);
        return 1;
    }

    // initialize city id by -1 to check duplication
    for (i = 0; i < city_cnt_temp; i++)
    {
        (*city_info_ptr)[i].id_cidade = -1;
    }
    
    while (cnt < city_cnt_temp && fgets(buffer, sizeof(buffer), fp))
    {
        cnt++;
        remove_whitespace(buffer);
        len = strlen(buffer);

        if (len < 3)
        {
            continue;
        }

        while (len-- > 0)
        {
            if (buffer[len] == ' ' || buffer[len] == '\t')
            {
                type_start = &(buffer[len]);
                break;
            }
        }

        if (!type_start)
        {
            continue;
        }
        
        if (!is_valid_int(type_start + 1))
        {
            continue;
        }
        type = atoi(type_start + 1);

        if (type > 1 || type < 0)
        {
            continue;
        }

        *type_start = '\0';

        char* name_start = strpbrk(buffer, " \t");
        if (!name_start)
        {
            continue;
        }

        name_len = type_start - (name_start + 1);
        if (name_len <= 0)
        {
            continue;
        }

        *name_start = '\0';

        if (!is_valid_int(buffer))
        {
            continue;
        }
        id = atoi(buffer); 

        if (id < 0 || id >= city_cnt_temp)
        {
            fprintf(stderr, "Invalid city ID: %d, line %d of the file, %s:%d\n", id, cnt - 1, __func__, __LINE__);
            fclose(fp);
            free(*city_info_ptr);
            free(capital_buf);
            return 1;
        }

        if ((*city_info_ptr)[id].id_cidade != -1)
        {
            fprintf(stderr, "City ID duplicated: %d, line %d of the file, %s:%d\n", id, cnt - 1, __func__, __LINE__);
            fclose(fp);
            free(*city_info_ptr);
            free(capital_buf);
            return 1;
        }

        strncpy((*city_info_ptr)[id].nome_cidade, name_start + 1, name_len); 
        remove_whitespace((*city_info_ptr)[id].nome_cidade); 
         
        if ((*city_info_ptr)[id].nome_cidade[0] == '\0')
        {
            continue;
        }

        (*city_info_ptr)[id].id_cidade = id;
        (*city_info_ptr)[id].eh_capital = type;
        if (type == 1)
        {
            (*city_info_ptr)[id].drone_disponivel = 1;
            capital_buf[capital_cnt] = id;
            capital_cnt++;
        }
    }
    
    for (i = 0; i < cnt; ++i)
    {
        if ((*city_info_ptr)[i].id_cidade == -1)
        {
            fprintf(stderr, "City ID omitted: %d, %s:%d", i, __func__, __LINE__);
            fclose(fp);
            free(*city_info_ptr);
            free(capital_buf);
            return 1;
        }
    }

    if (cnt < city_cnt_temp || capital_cnt == 0)
    {
        fclose(fp);
        free(*city_info_ptr);
        free(capital_buf);
        return 1;
    }
    
    if (capitals_ptr && capital_cnt_ptr)
    {
        *capitals_ptr = (int *)calloc(capital_cnt, sizeof(int));
        if (*capitals_ptr == NULL)
        {
            fprintf(stderr, "calloc() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
            fclose(fp);
            free(*city_info_ptr);
            free(capital_buf);
            return 1;
        }

        *capital_cnt_ptr = capital_cnt;
        for (i = 0; i < capital_cnt; i++)
        {
            (*capitals_ptr)[i] = capital_buf[i];
        }
    }

    // success
    if (!adj_matrix_ptr)
    {
        free(capital_buf);
        fclose(fp);
        return 0;
    }
   
    *adj_matrix_ptr = (int**)calloc(city_cnt_temp, sizeof(unsigned int*));
    if (!(*adj_matrix_ptr))
    {
        fprintf(stderr, "calloc() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
        fclose(fp);
        free(capital_buf);
        free(*city_info_ptr);
        return 1;
    }

    for (i = 0; i < city_cnt_temp; i++)
    {
        (*adj_matrix_ptr)[i] = (int*)calloc(city_cnt_temp, sizeof(unsigned int));
        if (!(*adj_matrix_ptr)[i])
        {
            fprintf(stderr, "calloc() failed (%s): %s:%d\n", strerror(errno), __func__, __LINE__);
            for (j = 0; j < i; j++)
            {
                free((*adj_matrix_ptr)[j]);
            }
            free(*adj_matrix_ptr);
            free(*city_info_ptr);
            free(capital_buf);
            fclose(fp);
            return 1;
        }
    }
    
    for (i = 0; i < city_cnt_temp; i++)
    {
        for (j = 0; j < city_cnt_temp; j++)
        {
            (*adj_matrix_ptr)[i][j] = (i == j) ? 0 : INF;
        }
    }

    cnt = 0;
    while (fgets(buffer, sizeof(buffer), fp))
    {
        remove_whitespace(buffer);

        v1_ptr = strtok_r(buffer, " \t", &save_ptr);
        v2_ptr = strtok_r(NULL, " \t", &save_ptr);
        w_ptr = strtok_r(NULL, " \t", &save_ptr);

        if (!(v1_ptr && v2_ptr && w_ptr))
        {
            continue;
        }

        if (!(is_valid_int(v1_ptr) && is_valid_int(v2_ptr) && is_valid_int(w_ptr)))
        {
            continue;
        } 
        
        v1 = atoi(v1_ptr);
        v2 = atoi(v2_ptr);
        w = atoi(w_ptr);

        if (v1 < 0 || v1 >= city_cnt_temp || v2 < 0 || v2 >= city_cnt_temp || w <= 0)
        {
            continue;
        }

        (*adj_matrix_ptr)[v1][v2] = w;
        (*adj_matrix_ptr)[v2][v1] = w;

        cnt++;
    }

    if (cnt != edge_cnt)
    {
        // warning
        fprintf(stderr, "Warning: Wrong edge count: %d(%d expected), %s:%d\n", cnt, edge_cnt, __func__, __LINE__);
    }

    free(capital_buf);

    fclose(fp);
    return 0;
}

/*
 * The function tries to send the given buffer for multiple times if failed.
 * Checks ack_flag's value. If the flag is 1 and timeout not occur, it won't try to send more.
 */
int sendto_with_retry(int sock, const void* buf, size_t len, struct sockaddr* addr, socklen_t addr_len, const char* message, pthread_mutex_t* mutex, pthread_cond_t* cond, int* ack_flag, int timeout, int* is_running_ptr)
{
    int cnt = 0;
    int is_okay = 0;
    struct timespec ts = {};
    int check_timeout;
    
    if (!(buf && addr && cond && mutex && ack_flag))
    {
        fprintf(stderr, "Wrong parameter : %s:%d\n", __func__, __LINE__);
        return 0;
    } 

    while (*is_running_ptr && (++cnt <= NUM_RETRY && is_okay == 0))
    {
        sendto(sock, buf, len, 0, (struct sockaddr *)addr, addr_len);
        if (message)
        {
            printf("%s (tentativa %d/%d)\n", message, cnt, NUM_RETRY);
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout;
        check_timeout = 0;
        pthread_mutex_lock(mutex);
        while (*ack_flag == 0 && check_timeout != ETIMEDOUT)
        {
            check_timeout = pthread_cond_timedwait(cond, mutex, &ts);
        }
        if (*ack_flag)
        {
            *ack_flag = 0;
            is_okay = 1;
        }
        pthread_mutex_unlock(mutex);
    }
    
    return is_okay;
}

/*
 *  'Sleep' function to prevent long waiting when the program ends.
 *  Signal handler will call pthread_cond_broadcast() to unblock pthread_cond_timedwait(). 
 */
void sleep_to_be_awaken(int secs, int* is_running_ptr, pthread_mutex_t* mutex,  pthread_cond_t* cond)
{
    struct timespec ts;
    int result;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += secs;

    pthread_mutex_lock(mutex);
    while (*is_running_ptr)
    {
        result = pthread_cond_timedwait(cond, mutex, &ts);
        if (result == ETIMEDOUT)
        {
            break;
        }
    }
    pthread_mutex_unlock(mutex);
}

int enqueue(event_queue* queue, int id_cidade, int id_equipe)
{
    event_node* temp; 
    if (!queue)
    {
        fprintf(stderr, "Wrong parameter: %s:%d\n", __func__, __LINE__);
        return 1;
    }
    
    temp = (event_node*)malloc(sizeof(event_node));
    if (!temp)
    {
        fprintf(stderr, "malloc() failed: %s:%d\n", __func__, __LINE__);
        return 1;
    }
    
    temp->id_cidade = id_cidade;
    temp->id_equipe = id_equipe;
    temp->next = NULL;
    
    if (queue->head == NULL)
    {
        queue->head = queue->tail = temp;
    }
    else
    {
        queue->tail->next = temp;
        queue->tail = temp;
    }
    
    return 0;
}

int dequeue(event_queue* queue, event_node* output)
{
    event_node* temp;
    
    if (!queue)
    {
        fprintf(stderr, "Wrong parameter: %s:%d\n", __func__, __LINE__);
        return 1;
    }
    
    if (!queue->head)
    {
        fprintf(stderr, "The queue is empty: %s:%d\n", __func__, __LINE__);
        return -1;
    }
    
    temp = queue->head;
    queue->head = queue->head->next;
    
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }

    if (output)
    {
        output->id_cidade = temp->id_cidade;
        output->id_equipe = temp->id_equipe;
        output->next = NULL;
    }

    free(temp);
    
    return 0;
}


void free_queue(event_queue* queue)
{
    event_node* temp;

    if (!queue)
    {
        return;
    }

    while (queue->head != NULL)
    {
        temp = queue->head;
        queue->head = queue->head->next;
        free(temp);
    }
    queue->tail = NULL;
}