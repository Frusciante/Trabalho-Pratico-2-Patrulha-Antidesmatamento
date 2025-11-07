#include "util.h"

static int dijkstra(int vertex, int** adj_matrix, int* output_list, int num_vertices)
{
    if (!(adj_matrix && output_list) || vertex >= num_vertices)
    {
        return 1;
    }

    memset(output_list, INF, num_vertices);
    output_list[vertex] = 0;

    return 0;
}

int main(void)
{
    int** adj_matrix;
    info_cidade_t* city_info;
    int city_cnt;
    int i;

    get_info_from_file(FILENAME, &city_info, &city_cnt, &adj_matrix);

    free(city_info);
    for (i = 0; i < city_cnt; i++)
    {
        free(adj_matrix[i]);
    } 
    free(adj_matrix);

    return 0;
}
