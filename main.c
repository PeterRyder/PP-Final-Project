// Assignment 4/5
// Peter Ryder, Brian Kovacik, Matt Holmes

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include "clcg4.h"
#include <mpi.h>

#define ARRAY_TYPE unsigned int
#define MULTIPLIER 1000

unsigned int g_array_size = 0;
unsigned int g_ints_per_rank = 0;

int g_my_rank = -1;
int g_commsize = -1;

ARRAY_TYPE *g_array = NULL;

void generate_array();
void print_array();
void cleanup();

int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Wrong arguments\n");
		printf("usage: assignment4 [matrix_size]");
		return -1;
	}

#if DEBUG
	printf("Got %d argument(s)\n", argc);

	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
#endif

	g_array_size = atoi(argv[1]);

	if (g_array_size <= 0) {
		printf("Bad arguments\n");
		return(-1);
	}

	/* initialize the rng */
	InitDefault();

	/* initialize MPI */
	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &g_commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_my_rank);

    /* how many ints each rank is responsible for */
    g_ints_per_rank = g_array_size / g_commsize;

#if DEBUG
    if (g_my_rank == 0)
    	printf("Ints per rank: %d\n", g_ints_per_rank);
#endif

#if DEBUG
    if (g_my_rank == 0)
		printf("Running simulation with %d ranks\n", g_commsize);
#endif

#if DEBUG
    printf("Rank %d of %d has been started and a first Random Value of %lf\n", 
        g_my_rank, g_commsize, GenVal(g_my_rank));
#endif

    /* initialize the array with rank 0 */
	if (g_my_rank == 0) {
		generate_array();
	}

#if DEBUG
	print_array();
#endif

    MPI_Barrier(MPI_COMM_WORLD);

    if (g_my_rank == 0) {
   		cleanup();
    }
    

 	MPI_Finalize();

	return 0;
}

/* generates a random unsorted array */
void generate_array() {
	g_array = calloc(g_array_size, sizeof(ARRAY_TYPE));

	for (unsigned int i = 0; i < g_array_size; i++) {
		g_array[i] = GenVal(g_my_rank) * MULTIPLIER;
	}
}

/* prints the array - only helpful with small debug runs */
void print_array() {
	for (unsigned int i = 0; i < g_array_size; i++) {
		printf("%d,", g_array[i]);
	}
	printf("\n");
}

/* destroys the array */
void cleanup() {
	free(g_array);
}