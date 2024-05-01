// Include necessary libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_INTS_PER_LINE 10
#define MAX_NODES 693
#define INF INT_MAX

// Structure to represent an edge in the graph
struct Edge {
    int dest;
    int weight;
    struct Edge* next;
    int edgeCount;  // Add edgeCount field
};

// Structure to represent a node in the priority queue
struct Node {
    int vertex;
    int distance;
};

// Function to create a new edge
struct Edge* createEdge(int dest, int weight) {
    struct Edge* newEdge = (struct Edge*)malloc(sizeof(struct Edge));
    newEdge->dest = dest;
    newEdge->weight = weight;
    newEdge->next = NULL;
    return newEdge;
}

// Function to add an edge to the adjacency list
void addEdge(struct Edge* graph[], int src, int dest, int weight) {
    struct Edge* newEdge = createEdge(dest, weight);
    newEdge->next = graph[src];
    graph[src] = newEdge;
    graph[src]->edgeCount++;  // Increment edgeCount for the source node
}

// // Function to print the shortest path
// void printPath(int parent[], int v) {
//     if (parent[v] == -1) {
//         printf("%d ", v);
//         return;
//     }
//     else {
//         printPath(parent, parent[v]);
//         printf("%d ", v);
//     }
// }

// Global file pointer
FILE* file;

void printPath(int parent[], int v) {
    FILE* file = fopen("Path.txt", "a");  // Use "a" for append mode instead of "w" for write mode
    if (!file) {
        printf("Error opening file.\n");
        return;
    }

    if (parent[v] == -1) {
        printf("%d ", v);
        fprintf(file, "%d ", v);
    } else {
        printPath(parent, parent[v]);
        printf("%d ", v);
        fprintf(file, "%d ", v);
    }

    fclose(file);  // Close the file after writing
}



void lookupCharacters(const char* pathFile, const char* mappingFile, int rank) {
    // printf("Entered\n");
    // Only execute this block of code in the master process (rank 0)
    if (rank >= 0) {
        // printf("Entered if\n");

        // Open the mapping file for reading
        FILE* mapping = fopen(mappingFile, "r");
        if (!mapping) {
            printf("Error opening mapping file.\n");
            return;
        }

        // Read the header line
        char header[MAX_LINE_LENGTH];
        if (!fgets(header, MAX_LINE_LENGTH, mapping)) {
            printf("Error reading mapping file.\n");
            fclose(mapping);
            return;
        }

        // Open the path file for reading
        FILE* path = fopen(pathFile, "r");
        if (!path) {
            printf("Error opening path file.\n");
            fclose(mapping);
            return;
        }

        // Check if the mapping file is empty
        fseek(path, 0, SEEK_END);
        if (ftell(path) == 0) {
            printf("Path file is empty.\n");
            fclose(path);
            return;
        }
        fseek(path, 0, SEEK_SET);

        // Read each line from the path file
        char line[MAX_LINE_LENGTH];
        while (fgets(line, MAX_LINE_LENGTH, path)) {
            // printf("Entered while\n");
            char* token = strtok(line, " ");
            int integers[MAX_INTS_PER_LINE];
            int count = 0;
            while (token != NULL && count < MAX_INTS_PER_LINE) {
                // Convert the token to an integer
                integers[count++] = atoi(token);
                token = strtok(NULL, " ");
            }
            // printf("g");
            // Search for each integer in the mapping file
            for (int i = 0; i < count; i++) {
                fseek(mapping, 0, SEEK_SET); // Reset file pointer to beginning
                while (fgets(line, MAX_LINE_LENGTH, mapping)) {
                    // Parse the line to get the character name and integer
                    char* characterName = strtok(line, ",");
                    if (characterName == NULL) {
                        printf("Error parsing mapping file.\n");
                        continue;
                    }

                    // Get the integer
                    token = strtok(NULL, ",");
                    if (token == NULL) {
                        printf("Error parsing mapping file.\n");
                        continue;
                    }
                    int characterInteger = atoi(token);
                    // printf("d");
                    // Check if the integers match
                    if (characterInteger == integers[i]) {
                        printf("%s ", characterName);
                        printf(" -> ");
                        break; // Found the character, move to the next integer
                    }

                }
            }
            printf("END \n");
        }
        // printf(" END ");
        // Close the files
        fclose(path);
        fclose(mapping);
    }
}



void kthShortestPath(struct Edge* graph[], int k, int rank, int size) {
    // Priority queue to store nodes
    struct Node* pq = (struct Node*)malloc(MAX_NODES * sizeof(struct Node));
    int pqSize = 0;

    // Initialize distance array and parent array
    int distance[MAX_NODES + 1];
    int parent[MAX_NODES + 1];
    for (int i = 0; i <= MAX_NODES; i++) {
        distance[i] = INF;
        parent[i] = -1;
    }

    // Initialize the source node (node 1)
    distance[1] = 0;
    pq[pqSize].vertex = 1;
    pq[pqSize].distance = 0;
    pqSize++;

    #pragma omp parallel
    {
        while (pqSize > 0) {
            // Get the node with the minimum distance from the priority queue
            int u = pq[0].vertex;
            int d = pq[0].distance;

            // Remove the minimum node from the priority queue
            #pragma omp critical
            {
                pqSize--;
                pq[0] = pq[pqSize];
            }

            // Fix the heap property
            int i = 0;
            while (true) {
                int leftChild = 2 * i + 1;
                int rightChild = 2 * i + 2;
                int smallest = i;

                if (leftChild < pqSize && pq[leftChild].distance < pq[smallest].distance) {
                    smallest = leftChild;
                }

                if (rightChild < pqSize && pq[rightChild].distance < pq[smallest].distance) {
                    smallest = rightChild;
                }

                if (smallest != i) {
                    struct Node temp = pq[i];
                    pq[i] = pq[smallest];
                    pq[smallest] = temp;
                    i = smallest;
                } else {
                    break;
                }
            }

            // Relax edges from u
            struct Edge* cur = graph[u];
            while (cur != NULL) {
                int v = cur->dest;
                int w = cur->weight;
                if (distance[u] + w < distance[v]) {
                    distance[v] = distance[u] + w;
                    parent[v] = u;

                    #pragma omp critical
                    {
                        pq[pqSize].vertex = v;
                        pq[pqSize].distance = distance[v];
                        pqSize++;
                    }

                    // Fix the heap property ... (similar to the above code)
                }
                cur = cur->next;
            }
        }
    }

    // Print the k-th shortest path (only in the master process)
    if (rank == 0) {
        for (int i = 1; i <= MAX_NODES; i++) {
        printf("Parent of node %d: %d\n", i, parent[i]);
        }
        printf("Kth shortest path: ");
        printPath(parent, MAX_NODES);
        printf("\n");
    }

    free(pq);
}


// Initialize the graph array elements to NULL
void initializeGraph(struct Edge * graph[]) {
    for (int i = 0; i <= MAX_NODES; i++) {
        graph[i] = NULL;
        //printf("NULL");
    }
}


// Main function
int main(int argc, char* argv[]) {

    // Initialize MPI
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Read the graph from the file
    struct Edge* graph[MAX_NODES + 1];
    initializeGraph(graph); // Initialize the graph
    FILE* file = fopen("mapped.txt", "r");
    if (file == NULL) {
        printf("Error opening the file.\n");
        MPI_Finalize();
        return 1;
    }
    int u, v;
    while (fscanf(file, "%d %d", &u, &v) != EOF) {
        addEdge(graph, u, v, 1);
    }
    fclose(file);

    FILE* file1 = fopen("Path.txt", "w");
    fclose(file1);

    // Find the kth shortest path
    int k = 1; // Assuming k is 1 for simplicity
    
    kthShortestPath(graph, k, rank, size);
    
    const char* pathFile = "Path.txt";
    const char* mappingFile = "character_integer_mapping.csv";

    if (rank == 0) {
        printf("\n\n Kth PATH : ");
        lookupCharacters(pathFile, mappingFile, rank);
        printf("\n\n");
    }

    // Finalize MPI
    MPI_Finalize();

    return 0;
}
