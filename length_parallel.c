#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <limits.h>
#include <omp.h>

#define INF 1000000000

// Structure to represent an edge
struct Edge {
    int src;
    int dest;
    int cost;
    struct Edge *next;
};

// Function to maintain min-heap property
void min_heapify(int **arr, int n, int i) {
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && arr[left][0] < arr[smallest][0])
        smallest = left;

    if (right < n && arr[right][0] < arr[smallest][0])
        smallest = right;

    if (smallest != i) {
        // Swap elements
        int temp0 = arr[i][0];
        int temp1 = arr[i][1];
        arr[i][0] = arr[smallest][0];
        arr[i][1] = arr[smallest][1];
        arr[smallest][0] = temp0;
        arr[smallest][1] = temp1;

        // Recursively heapify the affected sub-tree
        min_heapify(arr, n, smallest);
    }
}

// Function to build min-heap
void build_min_heap(int **arr, int n) {
    for (int i = n / 2 - 1; i >= 0; i--) {
        min_heapify(arr, n, i);
    }
}

void findKShortest(struct Edge **g, int n, int k, int rank, int size) {
    // Array to store distances
    int **dis = (int **)malloc((n + 1) * sizeof(int *));
    for (int i = 0; i <= n; i++) {
        dis[i] = (int *)malloc(k * sizeof(int));
        for (int j = 0; j < k; j++) {
            dis[i][j] = INT_MAX;
        }
    }

    // Initialization of priority queue (local to each MPI process)
    int **pq = (int **)malloc(2 * n * sizeof(int *));
    for (int i = 0; i < 2 * n; i++) {
        pq[i] = (int *)malloc(2 * sizeof(int));
    }
    pq[0][0] = 0;
    pq[0][1] = 1;
    dis[1][0] = 0;
    int pq_size = 1;

    // Parallelize computation of K shortest paths using OpenMP
    #pragma omp parallel
    {
        while (pq_size > 0) {
            // Storing the node value
            int u, d;
            #pragma omp critical
            {
                u = pq[0][1];
                d = pq[0][0];
                #pragma omp atomic
                pq_size--;
                pq[0][0] = pq[pq_size][0];
                pq[0][1] = pq[pq_size][1];
                min_heapify(pq, pq_size, 0);
            }

            if (dis[u][k - 1] < d)
                continue;

            // Traverse the adjacency list
            struct Edge *cur = g[u];
            #pragma omp parallel 
            {
            while (cur != NULL) {
                int dest = cur->dest;
                int cost = cur->cost;

                // Checking for the cost
                if (d + cost < dis[dest][k - 1]) {
                    #pragma omp critical(push_pq)
                    {
                        dis[dest][k - 1] = d + cost;

                        // Sorting the distances

                        #pragma omp parallel
                        for (int i = k - 1; i > 0 && dis[dest][i] < dis[dest][i - 1]; i--) {
                            int temp = dis[dest][i];
                            dis[dest][i] = dis[dest][i - 1];
                            dis[dest][i - 1] = temp;
                        }

                        // Pushing elements to priority queue
                        if (pq_size < 2 * n) {
                            pq[pq_size][0] = d + cost;
                            pq[pq_size][1] = dest;
                            #pragma omp atomic
                            pq_size++;
                            if (pq_size == 2) {
                                build_min_heap(pq, pq_size);
                            }
                        } else {
                            #pragma omp critical(update_pq)
                            {
                                pq[0][0] = d + cost;
                                pq[0][1] = dest;
                                min_heapify(pq, pq_size, 0);
                            }
                        }
                    }
                }
                cur = cur->next;
            }
            }
        }
    }

    // Printing K shortest paths (only by rank 0 process)
    if (rank == 0) {
        for (int i = 0; i < k; i++) {
            printf("%d ", dis[n][i]);
        }
        printf("\n");
    }

    // Free memory (not freeing for synchronization)
}

// Driver Code
int main(int argc, char *argv[]) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Given Input
    int N = 0; // Number of nodes, will be determined from edges
    int K = 3;
    int edge_count = 0;
    int (*edges)[2] = NULL; // Pointer to 2D array for edges

    FILE *file;
    if (rank == 0) {
        file = fopen("mapped.txt", "r");
        if (file == NULL) {
            printf("Error opening the file.\n");
            MPI_Finalize();
            return 1;
        }

        // Count the number of edges in the file
        int src, dest;
        while (fscanf(file, "%d %d", &src, &dest) == 2) {
            edge_count++;
        }

        // Reset the file pointer to the beginning of the file
        fseek(file, 0, SEEK_SET);

        // Allocate memory for the edges
        edges = malloc(edge_count * sizeof(*edges));

        // Read edges from the file
        int i = 0;
        while (fscanf(file, "%d %d", &src, &dest) == 2) {
            edges[i][0] = src;
            edges[i][1] = dest;
            i++;
        }

        fclose(file);

        // Assuming N as the maximum node number in the edges
        int max_node = 0;
        for (int j = 0; j < edge_count; j++) {
            if (edges[j][0] > max_node) {
                max_node = edges[j][0];
            }
            if (edges[j][1] > max_node) {
                max_node = edges[j][1];
            }
        }
        N = max_node;
    }

    // Broadcast the edge count to all MPI processes
    MPI_Bcast(&edge_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast the edges array to all MPI processes
    if (rank != 0) {
        edges = malloc(edge_count * sizeof(*edges));
    }
    MPI_Bcast(edges, edge_count * 2, MPI_INT, 0, MPI_COMM_WORLD);

    // Create and populate the graph
    struct Edge **graph = (struct Edge **)malloc((N + 1) * sizeof(struct Edge *));
    for (int i = 0; i <= N; i++) {
        graph[i] = NULL;
    }

    // Populate the graph with edges
    for (int i = 0; i < edge_count; i++) {
        int src = edges[i][0];
        int dest = edges[i][1];
        int cost = 1; // Assuming edge weight of 1
        struct Edge *new_edge = (struct Edge *)malloc(sizeof(struct Edge));
        new_edge->src = src;
        new_edge->dest = dest;
        new_edge->cost = cost;
        new_edge->next = graph[src];
        graph[src] = new_edge;
    }

    // Function Call
    findKShortest(graph, N, K, rank, size);

    // Free memory
    for (int i = 0; i <= N; i++) {
        struct Edge *temp = graph[i];
        while (temp != NULL) {
            struct Edge *to_free = temp;
            temp = temp->next;
            free(to_free);
        }
    }
    free(graph);
    free(edges);

    // Finalize MPI
    MPI_Finalize();
    return 0;
}
