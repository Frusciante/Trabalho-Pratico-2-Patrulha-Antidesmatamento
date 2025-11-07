#include "util.h"

static int dijkstra(int vertex, const unsigned int** const adj_matrix, unsigned* output_arr, int* visited_arr, int num_vertices)
{
    int i, j;
    unsigned int closest = INF;
    unsigned int closest_idx;
    int w;

    if (!(adj_matrix && output_arr) || vertex >= num_vertices)
    {
        return 1;
    }
    
    memset(visited_arr, 0, num_vertices * sizeof(int));
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
    int city_cnt;
    int i;

    get_info_from_file(FILENAME, &city_info, &city_cnt, &adj_matrix);
    unsigned int* dist_list = (unsigned int*)malloc(city_cnt * sizeof(unsigned int));
    if (!dist_list)
    {
        return 1;
    }
    
    int* visited = (int*)malloc(city_cnt * sizeof(int));
    if (!visited)
    {
        return 1;
    }

    for (;;)
    {
    }

    free(city_info);
    for (i = 0; i < city_cnt; i++)
    {
        free(adj_matrix[i]);
    } 
    free(adj_matrix);

    return 0;
}
