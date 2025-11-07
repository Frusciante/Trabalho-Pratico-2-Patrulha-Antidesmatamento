#include "util.h"

void remove_whitespace(char* str)
{
    char* start = str;
    char* end;
    
    if (!str)
    {
        return;
    }
    
    end = str + (strlen(str) - 1);
    
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
        memmove(str, start, strlen(start) + 1);
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
    if parameter 'adj_matrix_ptr' is NULL, this function will not read edge informations
*/
int get_info_from_file(const char* const filename, info_cidade_t** city_info_ptr, unsigned int* city_cnt, unsigned int*** adj_matrix_ptr)
{
    FILE* fp = NULL;
    char buffer[256];
    char* save_ptr = buffer;
    char* num_city_ptr = NULL;
    char* num_edge_ptr = NULL;
    char* type_start = NULL;
    char* v1_ptr;
    char* v2_ptr;
    char* w_ptr;
    int len;
    int type;
    int id;
    int edge_cnt;
    int city_cnt_temp;
    unsigned int cnt = 0;
    int name_len;
    int v1, v2, w;
    unsigned int i, j;

    if (!(filename && city_info_ptr && city_cnt))
    {
        return 1;
    }

    fp = fopen(filename, "rt"); 
    if (!fp)
    {
        return 1;
    }

    fgets(buffer, sizeof(buffer), fp);
    num_city_ptr = strtok_r(buffer, " \t", &save_ptr);
    num_edge_ptr = strtok_r(NULL, " \t", &save_ptr);
    
    city_cnt_temp = atoi(num_city_ptr);
    edge_cnt = atoi(num_edge_ptr);
    
    if (city_cnt_temp <= 0 || edge_cnt < 0)
    {
        fclose(fp);
        return 1;
    }
    
    *city_cnt = city_cnt_temp;

    *city_info_ptr = (info_cidade_t*)calloc(city_cnt_temp, sizeof(info_cidade_t));
    if (!(*city_info_ptr))
    {
        fclose(fp);
        return 1;
    }

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
            fclose(fp);
            free(*city_info_ptr);
            return 1;
        }

        if ((*city_info_ptr)[id].id_cidade != -1)
        {
            fclose(fp);
            free(*city_info_ptr);
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
    }

    if (cnt < city_cnt_temp)
    {
        fclose(fp);
        free(*city_info_ptr);
        return 1;
    }
    
    if (!adj_matrix_ptr)
    {
        fclose(fp);
        return 0;
    }
   
    *adj_matrix_ptr = (unsigned int**)calloc(city_cnt_temp, sizeof(unsigned int*));
    if (!(*adj_matrix_ptr))
    {
        fclose(fp);
        free(*city_info_ptr);
        return 1;
    }

    for (i = 0; i < city_cnt_temp; i++)
    {
        (*adj_matrix_ptr)[i] = (unsigned int*)calloc(city_cnt_temp, sizeof(unsigned int));
        if (!(*adj_matrix_ptr)[i])
        {
            for (j = 0; j < i; j++)
            {
                free((*adj_matrix_ptr)[j]);
            }
            free(*adj_matrix_ptr);
            free(*city_info_ptr);
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
    }

    fclose(fp);
    return 0;
}
