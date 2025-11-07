#include "util.h"

static int dijkstra(int vertex, int** adj_list, int* output_list, int num_vertices)
{
    if (!(adj_list && output_list) || vertex >= num_vertices)
    {
        return 1;
    }

    memset(output_list, INF, num_vertices);
    output_list[vertex] = 0;

    return 0;
}

int main(void)
{


    return 0;
}
